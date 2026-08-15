import psutil, pynvml, time, requests, sqlite3, os, shutil, ctypes

ESP_IP = "172.20.10.3"
DB_NAME = "monitor_stats.db"

CITIES = {
    "Lviv": {"lat": 49.84, "lon": 24.02, "name": "Lviv"},
    "Kyiv": {"lat": 50.45, "lon": 30.52, "name": "Kyiv"},
    "Borshchovychi": {"lat": 49.88, "lon": 24.28, "name": "Borshchovychi"},
    "Murovane": {"lat": 49.88, "lon": 24.08, "name": "Murovane"},
    "Odesa": {"lat": 46.48, "lon": 30.72, "name": "Odesa"},
    "Kharkiv": {"lat": 50.00, "lon": 36.23, "name": "Kharkiv"},
    "Dnipro": {"lat": 48.46, "lon": 35.04, "name": "Dnipro"},
    "Frankivsk": {"lat": 48.92, "lon": 24.71, "name": "Frankivsk"}
}

current_city_key = "Lviv"

def init_db():
    conn = sqlite3.connect(DB_NAME)
    conn.execute('''CREATE TABLE IF NOT EXISTS metrics 
                 (id INTEGER PRIMARY KEY AUTOINCREMENT, 
                  time TEXT, cpu REAL, gpu REAL, ram REAL, city TEXT, temp REAL)''')
    conn.commit()
    conn.close()

def clean_temp():
    folders = [os.environ.get('TEMP'), 'C:\\Windows\\Temp']
    for folder in folders:
        for filename in os.listdir(folder):
            file_path = os.path.join(folder, filename)
            try:
                if os.path.isfile(file_path) or os.path.islink(file_path): os.unlink(file_path)
                elif os.path.isdir(file_path): shutil.rmtree(file_path)
            except: pass
    print("🧹 Temp очищено!")

def lock_pc():
    ctypes.windll.user32.LockWorkStation()

def get_weather(city_key):
    try:
        c = CITIES[city_key]
        url = f"https://api.open-meteo.com/v1/forecast?latitude={c['lat']}&longitude={c['lon']}&current_weather=true"
        res = requests.get(url, timeout=3).json()
        temp = res['current_weather']['temperature']
        code = res['current_weather']['weathercode']
        status = "Clear" if code == 0 else "Clouds" if code < 50 else "Rain" if code < 80 else "Snow"
        return status, int(temp)
    except: return "Err", 0

init_db()
pynvml.nvmlInit()
gpu_handle = pynvml.nvmlDeviceGetHandleByIndex(0)
last_db_time = time.time()
buffer = {"cpu":[], "gpu":[], "ram":[]}

try:
    while True:
        cpu = psutil.cpu_percent(interval=0.5)
        ram = psutil.virtual_memory().percent
        gpu = pynvml.nvmlDeviceGetUtilizationRates(gpu_handle).gpu
        clock = time.strftime("%H:%M")
        
        weather_status, raw_temp = get_weather(current_city_key)
        
        buffer["cpu"].append(cpu); buffer["gpu"].append(gpu); buffer["ram"].append(ram)
        
        if time.time() - last_db_time > 60:
            avg_cpu = sum(buffer["cpu"])/len(buffer["cpu"])
            avg_gpu = sum(buffer["gpu"])/len(buffer["gpu"])
            avg_ram = sum(buffer["ram"])/len(buffer["ram"])
            conn = sqlite3.connect(DB_NAME)
            conn.execute("INSERT INTO metrics (time, cpu, gpu, ram, city, temp) VALUES (?,?,?,?,?,?)",
                        (time.strftime("%Y-%m-%d %H:%M"), avg_cpu, avg_gpu, avg_ram, current_city_key, raw_temp))
            conn.commit(); conn.close()
            buffer = {"cpu":[], "gpu":[], "ram":[]}; last_db_time = time.time()

        payload = {
            "cpu": int(cpu), "gpu": int(gpu), "ram": int(ram),
            "clock": clock, 
            "weather_city": CITIES[current_city_key]['name'],
            "weather_temp": raw_temp,
            "weather_desc": weather_status
        }
        
        try:
            r = requests.post(f"http://{ESP_IP}/data", json=payload, timeout=1.5)
            resp = r.json()
            
            cmd = resp.get("cmd")
            if cmd == "clean": clean_temp()
            if cmd == "lock": lock_pc()
            
            new_city = resp.get("city")
            if new_city in CITIES: current_city_key = new_city
                
        except: print("❌ ESP Offline")
        
        time.sleep(1)

except KeyboardInterrupt:
    pynvml.nvmlShutdown()