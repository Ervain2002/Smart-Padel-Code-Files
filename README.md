# Padel Analyzer Pro 🎾

Padel Analyzer Pro on asjade interneti (IoT) ja servaarvutuse (Edge Computing) põhimõtetel töötav süsteem padeli löökide (kiirus, löögitüüp, täpsus) reaalajas jälgimiseks ja analüüsimiseks. 

Süsteem koosneb reketi külge kinnitatavast sensorist (XIAO nRF52840 + MPU6050) ning veebipõhisest kasutajaliidesest, mis suhtleb seadmega otse üle **Web Bluetooth API**. See tähendab, et kasutajal puudub vajadus eraldi mobiilirakenduse allalaadimiseks.

🌐 **Live versioon (koheseks kasutamiseks):** [padel-app-ev.netlify.app](https://padel-app-ev.netlify.app)

> **⚠️ Oluline märkus seoses iOS seadmetega:** 
> Süsteem kasutab Web Bluetooth API standardit, mis töötab veatult Android-seadmetes, lauaarvutites (Windows/Mac) ja Chrome'i brauseris. Apple'i suletud ökosüsteemi ja Safari/WebKit piirangute tõttu ei toeta iOS operatsioonisüsteem hetkel veebipõhist Bluetoothi otseühendust ega pilveintegratsiooni. **Palun kasutage rakendust Android seadme või sülearvutiga.**

---

## ⚙️ Kuidas süsteemi seadistada ja kasutada (Step-by-Step Guide)

### 1. Riistvara seadistamine ja C++ koodi laadimine
Kui soovid ehitada oma sensori, järgi neid samme:
1. **Riistvara kokkupanek:** Ühenda MPU6050 sensor Seeed Studio XIAO nRF52840 mikrokontrolleriga (I2C liides: SDA ja SCL). Ühenda toiteks 75mAh Li-Po aku `BAT+` ja `BAT-` klemmidele, lisades positiivsele ahelale 3-viigulise lüliti.
2. **Arenduskeskkond:** Laadi alla ja paigalda [Arduino IDE](https://www.arduino.cc/en/software). Lisa "Boards Manager" alt Seeed nRF52840 plaadi tugi.
3. **Teekide (Libraries) paigaldamine:**
   * `ArduinoBLE` (Bluetooth andmesideks)
   * `Adafruit MPU6050` (Kiirenduse ja güroskoobi lugemiseks)
   * `Adafruit Unified Sensor`
4. **Koodi üleslaadimine:** Ava repositooriumis asuv C++ fail, ühenda XIAO mikrokontroller USB-C kaabliga arvutisse ja vajuta Arduino IDE-s nuppu **Upload**.

### 2. Veebirakenduse käivitamine (`index.html`)
Kuna rakendus kasutab riistvara komponente, on brauseri turvapoliitika tõttu **HTTPS** ühendus kohustuslik.

**A) Lihtsaim viis (Avalik veebileht):**
Mine lihtsalt aadressile [padel-app-ev.netlify.app](https://padel-app-ev.netlify.app). Faili pole vaja alla laadida.

**B) Lokaalne arendus (Lokaalne masin):**
Kui soovid koodi muuta ja testida oma arvutis:
1. Laadi alla siin repositooriumis olev `index.html` fail.
2. Käivita fail lokaalses serveris (nt Pythoniga: `python -m http.server` või kasuta VS Code *Live Server* laiendust).
3. Ava brauseris aadress `http://localhost:8000` (*Localhost* on ainus turvaerand, kus brauser lubab Web Bluetoothi ilma HTTPS-ita kasutada).

### 3. Süsteemi kasutamine padeliväljakul 🚀
1. **Lülita seade sisse:** Lükka reketi sensoril olev lüliti **ON** asendisse. Mikrokontrolleril süttib roheline indikaatortuli.
2. **Ava rakendus:** Mine oma nutiseadmes avalikule veebilehele (padel-app-ev.netlify.app).
3. **Ühenda sensor:** Vajuta esilehel nuppu **"Connect Sensor"**.
4. **Sidumine (Pairing):** Brauser avab hüpikakna. Vali nimekirjast enda sensor ja vajuta ühenda.
5. **Andmete jälgimine:** Alusta mängimist! Ekraanile ilmuvad reaalajas sinu löögi tüüp (Flat, Topspin, Slice), kiirus (km/h) ja tabamuse täpsus (Sweetspot vs Off-Center). Samuti uuenevad visuaalsed graafikud automaatselt iga löögi järel.
6. **Andmete eksport:** 
   * Vajuta vasakul üleval asuvale **menüü nupule (☰)**.
   * Sessiooni lokaalseks salvestamiseks vajuta **"Download CSV"**.
   * Sessiooni pilve saatmiseks (nõuab ühekordset Google'i kontoga sisselogimist nupu "Google Sign In" abil) vajuta **"Save to Drive"**. Fail salvestub automaatselt otse sinu isiklikku Google Drive'i!

---
*Loodud osana inseneriõppe lõputööst.*
