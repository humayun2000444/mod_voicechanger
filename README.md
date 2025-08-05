# mod_voicechanger — FreeSWITCH Module

A custom FreeSWITCH module for real-time voice morphing during active calls.

## 🛠️ Installation Guide

### 1. Clone the Repository

```bash
git clone https://github.com/humayun2000444/mod_voicechanger.git
cd mod_voicechanger/
git checkout female
```

### 2. Build the Module

```bash
mkdir build
cd build/
sudo cmake ..
sudo make
sudo make install
```

### 3. (Optional) Create FreeSWITCH User & Group

```bash
sudo groupadd freeswitch
sudo useradd -g freeswitch freeswitch
```

### 4. Install the Module

```bash
sudo cp mod_voicechanger.so /usr/local/freeswitch/mod/
sudo chown freeswitch:freeswitch /usr/local/freeswitch/mod/mod_voicechanger.so
```

### 5. Auto-load the Module on Startup

Edit the FreeSWITCH modules config:

```bash
sudo nano /etc/freeswitch/autoload_configs/modules.conf.xml
```

Add the following line inside the `<modules>` section:

```xml
<load module="mod_voicechanger"/>
```

### 6. Restart FreeSWITCH

```bash
sudo systemctl restart freeswitch
```

---

## 📞 Call Testing

### 1. Configure Two SIP Extensions

Ensure two SIP users are registered:

- **User 1**: 1000 / 1000
- **User 2**: 1001 / 1001

Use your FreeSWITCH IP and SIP port as the domain. To find it:

```bash
fs_cli -x "sofia status"
```

### 2. Make a Call Between Extensions

Check active calls and note the UUIDs:

```bash
fs_cli -x "show calls"
```

Identify the UUID of the **second call leg** to apply the voice change.

Example call legs:

```
a042ccca-eb38-4c92-bf3a-d5fd04d90417  <-- First UUID
68ad2658-0b35-44a0-ba7b-a408b7344063  <-- Second UUID (use this one)
```

### 3. Start / Stop Voice Morphing

#### Start Voice Changer

```bash
voicechanger start <UUID>
```

#### Stop Voice Changer

```bash
voicechanger stop <UUID>
```

Example:

```bash
voicechanger start 68ad2658-0b35-44a0-ba7b-a408b7344063
voicechanger stop  68ad2658-0b35-44a0-ba7b-a408b7344063
```

---

## ✅ Verification

To verify that the module loaded successfully:

```bash
fs_cli -x "show modules" | grep voicechanger
```

---

## 📌 Notes

- Ensure the module `.so` file is in `/usr/local/freeswitch/mod/`
- Check ownership: `freeswitch:freeswitch`
- Restart FreeSWITCH after adding module to autoload
