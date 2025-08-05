# 🎙️ mod_voicechanger — FreeSWITCH Module

A custom FreeSWITCH module for real-time **voice morphing** during active SIP calls.

---

## 🛠️ Installation Guide

### 1. Clone the Repository

Option A — via Git:
```bash
git clone https://github.com/humayun2000444/mod_voicechanger.git
cd mod_voicechanger
```

Option B — unzip the attached module if you're using a packaged release:
```bash
cd ~/humayun/freeswitch_mods/mod_voicechanger
```

Then, clone the required dependency:
```bash
git clone https://github.com/Signalsmith-Audio/linear.git
cd linear
mkdir -p signalsmith-linear
```

---

### 2. Build the Module

```bash
cd ~/humayun/freeswitch_mods/mod_voicechanger
mkdir build
cd build
cmake ../src
make
sudo make install
```

This will compile and install `mod_voicechanger.so`.

---

### 3. Install the Module Binary

```bash
sudo groupadd freeswitch
sudo useradd -g freeswitch freeswitch

sudo cp mod_voicechanger.so /usr/local/freeswitch/mod/
sudo chown freeswitch:freeswitch /usr/local/freeswitch/mod/mod_voicechanger.so
```

---

### 4. Auto-load the Module on Startup

Edit the module autoload config:
```bash
sudo nano /etc/freeswitch/autoload_configs/modules.conf.xml
```

Add the following line inside the `<modules>` section:
```xml
<load module="mod_voicechanger"/>
```

Then restart FreeSWITCH:
```bash
sudo systemctl restart freeswitch
```

---

## 📞 Usage Instructions

### 1. Create Two SIP Extensions

Ensure two SIP users are configured and registered:

- **User 1:**  
  - Username: `1000`  
  - Password: `1000`

- **User 2:**  
  - Username: `1001`  
  - Password: `1001`

📌 *Domain and server address:* Use your FreeSWITCH server IP and SIP port.  
Find it using:
```bash
fs_cli -x "sofia status"
```

---

### 2. Make a Call Between the Two Extensions

Place a call from `1000` to `1001` using a SIP client or FreeSWITCH CLI:
```bash
fs_cli -x "originate user/1000 user/1001"
```

Check active calls:
```bash
fs_cli -x "show calls"
```

Identify the UUID for the call leg you want to morph (typically the second one).

---

### 3. Use the Voice Changer Module

Start morphing:
```bash
voicechanger start <UUID> <factor>
```

Adjust morphing factor:
```bash
voicechanger set <UUID> <new_factor>
```

Stop morphing:
```bash
voicechanger stop <UUID>
```

#### Example:
```bash
voicechanger start 68ad2658-0b35-44a0-ba7b-a408b7344063 1.2
voicechanger set   68ad2658-0b35-44a0-ba7b-a408b7344063 1.5
voicechanger stop  68ad2658-0b35-44a0-ba7b-a408b7344063
```

---

## 📝 Notes

- **Morphing factors**:
  - `>1.0`: increases pitch (e.g., `1.2`, `1.5`)
  - `<1.0`: lowers pitch (e.g., `0.8`)

- **Make sure**:
  - The module is placed in `/usr/local/freeswitch/mod/`
  - The module has proper ownership (`freeswitch:freeswitch`)
  - FreeSWITCH was restarted after placing the module

- **Verify module loaded**:
```bash
fs_cli -x "show modules" | grep voicechanger
```
