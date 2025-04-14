#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const char* ssid = "MIWIFI_2G_HJWj";
const char* password = "GcbFXuMP";
const String apiKey = "eyJhbGciOiJIUzI1NiJ9.eyJzdWIiOiJ0ZWN4YXJ0QGdtYWlsLmNvbSIsImp0aSI6IjJhMzVlYzViLTdjYjAtNDQzYy1iNzk3LWNkOTk0YWY0MTJlNyIsImlzcyI6IkFFTUVUIiwiaWF0IjoxNzQ0NjI4Njk1LCJ1c2VySWQiOiIyYTM1ZWM1Yi03Y2IwLTQ0M2MtYjc5Ny1jZDk5NGFmNDEyZTciLCJyb2xlIjoiIn0.-6gC6-N0DdEzwKESaEULQgbJCkeH8sJI7L64v-7W7C0"; // Ej: "eyJhbGciOiJIUzI1NiJ9..."
const String codigoMunicipio = "26089";
const String codigoMunicipio2 = "28079";

void setup() {
  Serial.begin(115200);
  
  // Inicializar pantalla OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Error al iniciar pantalla OLED");
    while(1);
  }
  
  // Conectar a WiFi
  WiFi.begin(ssid, password);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Conectando WiFi...");
  display.display();
  
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  display.clearDisplay();
  display.println("WiFi conectado!");
  display.println(WiFi.localIP());
  display.display();
  delay(2000);
}

void loop() {
  if(WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    delay(5000);
    return;
  }

  // Primera petición para obtener URL de datos
  String url = "https://opendata.aemet.es/opendata/api/prediccion/especifica/municipio/diaria/" + codigoMunicipio + "?api_key=" + apiKey;
  
  HTTPClient http;
  http.begin(url);
  http.addHeader("Accept", "application/json");
  
  Serial.println("Haciendo petición a: " + url);
  int httpCode = http.GET();
  
  if(httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("Respuesta API:");
    Serial.println(payload);
    
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, payload);
    
    String urlDatos = doc["datos"].as<String>();
    http.end();
    
    if(urlDatos == "null" || urlDatos.isEmpty()) {
      Serial.println("Error: URL de datos inválida");
      mostrarError("URL inválida");
      return;
    }
    
    Serial.println("URL de datos: " + urlDatos);
    
    // Segunda petición para obtener datos meteorológicos
    HTTPClient httpDatos;
    httpDatos.begin(urlDatos);
    int httpCodeDatos = httpDatos.GET();
    
    if(httpCodeDatos == HTTP_CODE_OK) {
      String payloadDatos = httpDatos.getString();
      Serial.println("Datos meteorológicos:");
      Serial.println(payloadDatos);
      
      DynamicJsonDocument docDatos(8192);
      deserializeJson(docDatos, payloadDatos);
      
      // Extraer datos del primer día
      String nombreMunicipio = docDatos[0]["nombre"].as<String>();
      JsonObject dia = docDatos[0]["prediccion"]["dia"][0];
      String fecha = dia["fecha"].as<String>();
      int tempMax = dia["temperatura"]["maxima"];
      int tempMin = dia["temperatura"]["minima"];
      
      // Buscar primera descripción de estado del cielo no vacía
      String estadoCielo = "Desconocido";
      for(JsonObject cielo : dia["estadoCielo"].as<JsonArray>()) {
        if(cielo["descripcion"] != "") {
          estadoCielo = cielo["descripcion"].as<String>();
          break;
        }
      }
      
      // Mostrar en pantalla OLED
      mostrarDatos(nombreMunicipio, fecha, tempMax, tempMin, estadoCielo);
      
    } else {
      Serial.println("Error en datos HTTP: " + String(httpCodeDatos) + " - " + httpDatos.errorToString(httpCodeDatos));
      mostrarError("Error datos: " + String(httpCodeDatos));
    }
    httpDatos.end();
  } else {
    Serial.println("Error en API HTTP: " + String(httpCode) + " - " + http.errorToString(httpCode));
    mostrarError("Error API: " + String(httpCode));
  }
  
  delay(300000); // Esperar 1 minuto antes de actualizar
}

void mostrarDatos(String ubicacion, String fecha, int tempMax, int tempMin, String estadoCielo) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  
  // Mostrar ubicación (recortar si es muy larga)
  if(ubicacion.length() > 20) {
    ubicacion = ubicacion.substring(0, 17) + "...";
  }
  display.println("Ubic: " + ubicacion);
  
  // Mostrar fecha (formato YYYY-MM-DD)
  if(fecha.length() >= 10) {
    display.println("Fecha: " + fecha.substring(0, 10));
  } else {
    display.println("Fecha: " + fecha);
  }
  
  // Mostrar temperaturas
  display.println("Max: " + String(tempMax) + "C  Min: " + String(tempMin) + "C");
  
  // Mostrar estado del cielo (recortar si es muy largo)
  if(estadoCielo.length() > 20) {
    estadoCielo = estadoCielo.substring(0, 17) + "...";
  }
  display.println("Cielo: " + estadoCielo);
  
  display.display();
}

void mostrarError(String mensaje) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Error:");
  display.println(mensaje);
  display.display();
}
