package main

import (
	"crypto/tls"
	"database/sql"
	"encoding/json"
	"fmt"
	"html/template"
	"io"
	"log"
	"net/http"
	"os"
	"strconv"
	"strings"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
	_ "github.com/mattn/go-sqlite3"
)

type AppConfig struct {
	HttpPort          string
	MqttBrokerURL     string
	MqttClientId      string
	MqttUsername      string
	MqttPassword      string
	MqttTlsInsecure   bool
	BroadcastInterval time.Duration
	DbPath            string
}

type PumpData struct {
	PlantID   int       `json:"plant_id"`
	Action    int       `json:"action"`
	Timestamp time.Time `json:"timestamp"`
}

type SensorData struct {
	PlantID   int       `json:"plant_id"`
	Value     int       `json:"value"`
	Timestamp time.Time `json:"timestamp"`
}

type Device struct {
	ID        int64  `json:"id"`
	Name      string `json:"name"`
	SensorPin int    `json:"sensor_pin"`
	PumpPin   int    `json:"pump_pin"`
	Threshold int    `json:"threshold"`
	LastValue *int   `json:"last_value,omitempty"`
}

type GlobalConfig struct {
	ScanInterval int `json:"scan_interval"`
}

type PageData struct {
	Config  GlobalConfig
	Devices []Device
}

type FullSaveRequest struct {
	Config  *GlobalConfig `json:"config"`
	Devices []Device      `json:"devices"`
}

var db *sql.DB
var mqttClient mqtt.Client
var appCfg AppConfig

func initDB() {
	var err error
	db, err = sql.Open("sqlite3", appCfg.DbPath)
	if err != nil {
		log.Fatal(err)
	}

	sqlStmt := `
    CREATE TABLE IF NOT EXISTS sensor_logs (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        plant_id INTEGER NOT NULL,
        value INTEGER NOT NULL,
        timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
    );

    CREATE TABLE IF NOT EXISTS pump_logs (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        plant_id INTEGER NOT NULL,
        action INTEGER NOT NULL, -- 1 start, 0 stop
        timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
    );

    CREATE TABLE IF NOT EXISTS device (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL,
        sensor_pin INTEGER NOT NULL,
        pump_pin INTEGER NOT NULL,
        threshold INTEGER NOT NULL
    );

    CREATE TABLE IF NOT EXISTS global_config (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        scan_interval INTEGER NOT NULL
    );
    `
	_, err = db.Exec(sqlStmt)
	if err != nil {
		log.Printf("%q: %s\n", err, sqlStmt)
	}

	var count int
	err = db.QueryRow("SELECT COUNT(*) FROM global_config").Scan(&count)
	if err != nil || count == 0 {
		_, err = db.Exec("INSERT INTO global_config (scan_interval) VALUES (5000)")
		if err != nil {
			log.Fatal("Failed to create default config:", err)
		}
	}
}

var messageHandler mqtt.MessageHandler = func(client mqtt.Client, msg mqtt.Message) {
	payload := msg.Payload()

	switch msg.Topic() {
	case "roslinki/sensor_logs":
		var received SensorData
		err := json.Unmarshal(payload, &received)
		if err != nil {
			log.Printf("MQTT JSON error: %v | payload: %s\n", err, string(payload))
			return
		}
		saveSensorLog(received)
	case "roslinki/pump_logs":
		var received PumpData
		err := json.Unmarshal(payload, &received)
		if err != nil {
			log.Printf("MQTT JSON error: %v | payload: %s\n", err, string(payload))
			return
		}
		savePumpLog(received)
	}
}

func saveSensorLog(data SensorData) {
	stmt, err := db.Prepare("INSERT INTO sensor_logs(plant_id, value, timestamp) VALUES(?, ?, ?)")
	if err != nil {
		log.Println("SQL prepare error:", err)
		return
	}
	defer stmt.Close()

	ts := data.Timestamp
	if ts.IsZero() {
		ts = time.Now()
	}

	_, err = stmt.Exec(data.PlantID, data.Value, ts)
	if err != nil {
		log.Println("DB save error:", err)
	} else {
		fmt.Printf("Sensor saved: PlantID=%d Value=%d\n", data.PlantID, data.Value)
	}
}

func savePumpLog(data PumpData) {
	if data.Action != 0 {
		data.Action = 1
	}

	stmt, err := db.Prepare("INSERT INTO pump_logs(plant_id, action, timestamp) VALUES(?, ?, ?)")
	if err != nil {
		log.Println("SQL prepare error:", err)
		return
	}
	defer stmt.Close()

	ts := data.Timestamp
	if ts.IsZero() {
		ts = time.Now()
	}

	_, err = stmt.Exec(data.PlantID, data.Action, ts)
	if err != nil {
		log.Println("DB save error:", err)
	} else {
		fmt.Printf("Pump saved: PlantID=%d action=%d\n", data.PlantID, data.Action)
	}
}

func broadcastConfig() error {
	if mqttClient == nil || !mqttClient.IsConnected() {
		return fmt.Errorf("MQTT client is nil or not connected")
	}

	cfg, err := getGlobalConfig()
	if err != nil {
		return fmt.Errorf("getting global config: %v", err)
	}

	devs, err := getAllDevices()
	if err != nil {
		return fmt.Errorf("getting all devices: %v", err)
	}

	if devs == nil {
		devs = make([]Device, 0)
	}

	payload := FullSaveRequest{
		Config:  cfg,
		Devices: devs,
	}

	jsonBytes, err := json.Marshal(payload)
	if err != nil {
		return fmt.Errorf("Marshal: %v", err)
	}

	token := mqttClient.Publish("roslinki/config", 0, true, jsonBytes)
	token.Wait()
	if token.Error() != nil {
		return fmt.Errorf("MQTT Publish Error: %v", token.Error())
	}

	return nil
}

func getGlobalConfig() (*GlobalConfig, error) {
	var c GlobalConfig

	err := db.QueryRow("SELECT scan_interval FROM global_config ORDER BY id DESC LIMIT 1").Scan(&c.ScanInterval)
	if err != nil {
		return nil, err
	}

	return &c, err
}

func getAllDevices() ([]Device, error) {
	query := `
        SELECT
            d.id, d.name, d.sensor_pin, d.pump_pin, d.threshold,
            (SELECT value FROM sensor_logs s WHERE s.plant_id = d.id ORDER BY s.id DESC LIMIT 1) as last_val
        FROM device d
    `
	rows, err := db.Query(query)
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	var devices []Device
	for rows.Next() {
		var d Device
		var lastVal sql.NullInt64

		if err := rows.Scan(&d.ID, &d.Name, &d.SensorPin, &d.PumpPin, &d.Threshold, &lastVal); err != nil {
			return nil, err
		}

		if lastVal.Valid {
			val := int(lastVal.Int64)
			d.LastValue = &val
		} else {
			d.LastValue = nil
		}

		devices = append(devices, d)
	}
	return devices, nil
}

func updateAllData(data FullSaveRequest) error {
	tx, err := db.Begin()
	if err != nil {
		return err
	}
	defer tx.Rollback()

	_, err = tx.Exec("UPDATE global_config SET scan_interval = ? WHERE id = (SELECT MAX(id) FROM global_config)", data.Config.ScanInterval)
	if err != nil {
		log.Println("SQL Error Update Config:", err)
		return err
	}

	var keptIDs []interface{}
	for _, dev := range data.Devices {
		if dev.ID != 0 {
			keptIDs = append(keptIDs, dev.ID)
		}
	}

	if len(keptIDs) > 0 {
		placeholders := strings.Repeat("?,", len(keptIDs))
		placeholders = strings.TrimSuffix(placeholders, ",")

		query := fmt.Sprintf("DELETE FROM device WHERE id NOT IN (%s)", placeholders)
		_, err = tx.Exec(query, keptIDs...)
	} else {
		_, err = tx.Exec("DELETE FROM device")
	}

	if err != nil {
		log.Println("SQL Error Delete Devices:", err)
		return err
	}

	for _, dev := range data.Devices {
		if dev.ID == 0 {
			_, err = tx.Exec("INSERT INTO device (name, sensor_pin, pump_pin, threshold) VALUES (?, ?, ?, ?)",
				dev.Name, dev.SensorPin, dev.PumpPin, dev.Threshold)
		} else {
			_, err = tx.Exec("UPDATE device SET name=?, sensor_pin=?, pump_pin=?, threshold=? WHERE id=?",
				dev.Name, dev.SensorPin, dev.PumpPin, dev.Threshold, dev.ID)
		}
		if err != nil {
			log.Println("SQL Error Upsert Device:", err)
			return err
		}
	}

	return tx.Commit()
}

func handleIndex(w http.ResponseWriter, r *http.Request) {
	config, err := getGlobalConfig()
	if err != nil {
		fmt.Println("Failed to get global config:", err)
		http.Error(w, "Internal server error", 500)
		return
	}
	devices, err := getAllDevices()
	if err != nil {
		fmt.Println("Failed to get all devices:", err)
		http.Error(w, "Internal server error", 500)
		return
	}

	data := PageData{Config: *config, Devices: devices}
	tmpl, err := template.ParseFiles("index.html")
	if err != nil {
		fmt.Println("Template error:", err)
		http.Error(w, "Internal server error", 500)
		return
	}
	tmpl.Execute(w, data)
}

func handleApiSave(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", 405)
		return
	}

	body, _ := io.ReadAll(r.Body)
	var req FullSaveRequest
	err := json.Unmarshal(body, &req)
	if err != nil {
		http.Error(w, "Bad JSON", 400)
		return
	}

	err = updateAllData(req)
	if err != nil {
		log.Println("Save error:", err)
		http.Error(w, "Database Save Error", 500)
		return
	}

	go func() {
		if err := broadcastConfig(); err != nil {
			fmt.Println("Broadcast config failed", err)
		}
	}()

	w.Header().Set("Content-Type", "application/json")
	w.Write([]byte(`{"status":"ok"}`))
}

func getEnv(key string) string {
	val := os.Getenv(key)
	if val == "" {
		log.Panicf("CRITICAL ERROR: Missing required environment variable: %s", key)
	}
	return val
}

func getEnvAsInt(key string) int {
	valStr := getEnv(key)
	val, err := strconv.Atoi(valStr)
	if err != nil {
		log.Panicf("CRITICAL ERROR: Environment variable %s must be an integer, got: %s", key, valStr)
	}
	return val
}

func getEnvAsBool(key string) bool {
	valStr := os.Getenv(key)
	if valStr == "" {
		return false
	}
	val, err := strconv.ParseBool(valStr)
	if err != nil {
		log.Panicf("CRITICAL ERROR: Environment variable %s must be a boolean, got: %s", key, valStr)
	}
	return val
}

func loadConfig() {
	appCfg = AppConfig{
		HttpPort:          getEnv("HTTP_PORT"),
		MqttBrokerURL:     getEnv("MQTT_BROKER_URL"),
		MqttClientId:      getEnv("MQTT_CLIENT_ID"),
		MqttUsername:      getEnv("MQTT_USERNAME"),
		MqttPassword:      getEnv("MQTT_PASSWORD"),
		MqttTlsInsecure:   getEnvAsBool("MQTT_TLS_INSECURE"),
		BroadcastInterval: time.Duration(getEnvAsInt("BROADCAST_INTERVAL_SEC")) * time.Second,
		DbPath:            getEnv("DB_PATH"),
	}
}

func main() {
	loadConfig()

	initDB()
	defer db.Close()

	opts := mqtt.NewClientOptions()
	opts.AddBroker(appCfg.MqttBrokerURL)
	opts.SetClientID(appCfg.MqttClientId)
	opts.SetUsername(appCfg.MqttUsername)
	opts.SetPassword(appCfg.MqttPassword)

	opts.SetTLSConfig(&tls.Config{
		InsecureSkipVerify: appCfg.MqttTlsInsecure,
	})

	opts.SetDefaultPublishHandler(messageHandler)
	opts.SetKeepAlive(60 * time.Second)
	opts.SetPingTimeout(1 * time.Second)
	opts.SetAutoReconnect(true)

	mqttClient = mqtt.NewClient(opts)
	if token := mqttClient.Connect(); token.Wait() && token.Error() != nil {
		log.Println("MQTT connection error:", token.Error())
		os.Exit(1)
	}

	mqttClient.Subscribe("roslinki/sensor_logs", 0, nil)
	mqttClient.Subscribe("roslinki/pump_logs", 0, nil)

	go func() {
		ticker := time.NewTicker(appCfg.BroadcastInterval)
		defer ticker.Stop()
		for range ticker.C {
			if err := broadcastConfig(); err != nil {
				fmt.Println("Broadcast config failed", err)
			}
		}
	}()

	http.HandleFunc("/", handleIndex)
	http.HandleFunc("/api/save", handleApiSave)

	fmt.Printf("HTTP server is running on port %s\n", appCfg.HttpPort)
	log.Fatal(http.ListenAndServe(":"+appCfg.HttpPort, nil))
}
