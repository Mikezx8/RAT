from selenium import webdriver
from selenium.webdriver.chrome.options import Options
from selenium.webdriver.firefox.options import Options as FFOptions
import os

cloned_profiles = {
    "Chrome": os.path.join(os.getcwd(), "default"),
    "Edge": os.path.join(os.getcwd(), "default-e"),
    "Firefox": os.path.join(os.getcwd(), "default-fox")
}

available_browsers = {name: path for name, path in cloned_profiles.items() if os.path.exists(path)}

if not available_browsers:
    print("No cloned browser profiles found. Run the clone script first.")
    exit()

print("Select a browser to launch:")
for i, name in enumerate(available_browsers.keys(), start=1):
    print(f"{i}. {name}")

choice = int(input("Enter the number: ")) - 1
browser_name = list(available_browsers.keys())[choice]
profile_path = list(available_browsers.values())[choice]

if browser_name in ["Chrome", "Edge"]:
    options = Options()
    options.add_argument(f"--user-data-dir={profile_path}")
    options.add_argument("--profile-directory=Default")
    options.add_argument("--start-maximized")
    driver = webdriver.Chrome(options=options)
elif browser_name == "Firefox":
    options = FFOptions()
    options.set_preference("profile", profile_path)
    options.set_preference("browser.startup.homepage", "https://www.google.com")
    driver = webdriver.Firefox(options=options)

driver.get("https://www.google.com")
input("Press Enter to exit and close the browser...")
driver.quit()
