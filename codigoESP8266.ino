#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Configuración de red WiFi
const char* ssid = "[Ingresar nombre de la red]";
const char* password = "[Ingresar contraseña]";

// Configuración MQTT
const char* mqtt_server = "docker.coldspace.cl";
const int mqtt_port = 1883;
const char* mqtt_user = "";
const char* mqtt_password = "";

// Definimos el cliente
WiFiClient espClient;
PubSubClient client(espClient);

// Declaración de topicos
const char* topico1 = "/grupo13/humedad";
const char* topico2 = "/grupo13/nivel";
const char* topico3 = "/grupo13/bomba";

// Declaración de pines
const int Trigger = D4;
const int Echo = D5;
const int pinHumedad = A0;
const int pinBomba = D7;
const int pinBoton = D2;

// Declaracion de variables
int valorBomba = 0;
bool ejecutar = false;
int humedad = 0;
int nivel = 0;

// Funcion para configurar el programa inicial
void setup() {
  Serial.begin(9600);
  pinMode(Trigger, OUTPUT);
  pinMode(Echo, INPUT);
  pinMode(pinBomba, OUTPUT);
  pinMode(pinBoton, INPUT_PULLUP);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  digitalWrite(Trigger, LOW);
  digitalWrite(pinBomba, LOW);
}

// Funcion principal que ejecuta el codigo repetidamente
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  //Validacion de la bandera ejecutar
  if (ejecutar) {
    //Se ejecuta la funcion
    CodigoBomba(pinBomba);
    ejecutar = false; // Restablecer la bandera después de ejecutar la función
  }

  CodigoHumedad(pinHumedad);
  CodigoProximidad(Trigger, Echo);
  Seguro(pinBomba, pinBoton);
  RiegoAutomatico(humedad, nivel);
  delay(1000);
}

void Seguro(int pinBomba, int pinBoton){
  if(digitalRead(pinBoton) == LOW){
    Serial.println("Regando");
    digitalWrite(pinBomba, HIGH);
    delay(5000);
    digitalWrite(pinBomba, LOW);
    delay(5000);
  }
}

void RiegoAutomatico(int humedad, int nivel){
  
  if(humedad == 0 || nivel == 0){
    Serial.println("Error en el riego automatico");
  }else if(humedad <= 40 && nivel > 10){
    digitalWrite(pinBomba, HIGH);
    delay(5000);
    digitalWrite(pinBomba, LOW);
    delay(5000);
  }
}

// Funcion que muestra el codigo del sensor de proximidad
void CodigoProximidad(int trigger, int echo){
  //Declaramos las variables de tiempo (t) y distancia (d)

  long t;
  long d;

  //Enviamos un pulso con 10 microsegundos de delay
  digitalWrite(trigger, HIGH);
  delayMicroseconds(10);     
  digitalWrite(trigger, LOW);
  
  //Guardamos el pulso recibido en la variable tiempo (t)
  t = pulseIn(echo, HIGH);

  //Escalamos el tiempo (t) en cm obteniendo la distancia (d)
  d = t/59;

  //Validacion para comprobar el funcionamiento del sensor
  if(d == 0){
    //Caso en el que el sensor este desconectado
    send_mqtt_message("???", "/grupo13/nivel");
    send_mqtt_message("Sensor Desconectado", "/grupo13/estadoProximidad");
  }else if(d > 10){
    //Caso en el que el sensor este enviado datos erroneos
    send_mqtt_message("???", "/grupo13/nivel");
    send_mqtt_message("Sensor fallando", "/grupo13/estadoProximidad");
  }else{
    //Caso en el que el sensor este operando correctamente

    //Transformamos la distancia (d) obtenida en un porcentaje deseado
    int porcentajeDistancia = 100 - map(d, 1, 10, 0, 100);
    String resultado = String(porcentajeDistancia);
    nivel = porcentajeDistancia;

    send_mqtt_message(resultado, "/grupo13/nivel");
    send_mqtt_message("Sensor operando correctamente", "/grupo13/estadoProximidad");
  }
}


// Funcion callback para traer los datos de un topico en el nodered
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido en el tópico: ");
  Serial.println(topic);

  // Almacenar el valor del tópico3 en la variable global
  if (strcmp(topic, "/grupo13/bomba") == 0) {
    // Convertir el payload a un número entero
    char buffer[length + 1];
    strncpy(buffer, (char*)payload, length);
    buffer[length] = '\0'; // Asegurarse de terminar la cadena

    valorBomba = atoi(buffer);
    Serial.print("Último valor del tópico bomba: ");
    Serial.println(valorBomba);

    ejecutar = true;
    // Puedes realizar cualquier otra acción con el valor almacenado aquí
  }
}

// Funcion que controla la bomba
void CodigoBomba(int pinBomba) {
  if (valorBomba == 1) {
    digitalWrite(pinBomba, HIGH);
    delay(5000);
    digitalWrite(pinBomba, LOW);
    delay(5000);
    valorBomba = 0;
  }
}

// Funcion que muestra el codigo del sensor de humedad de suelo
void CodigoHumedad(int pinHumedad){
  //Obtenemos el dato de la resistencia del sensor
  int humedad = analogRead(pinHumedad);

  if(humedad <= 15){
    //Caso en el que el sensor este desconectado
    send_mqtt_message("???", "/grupo13/humedad");
    send_mqtt_message("Sensor Desconectado", "/grupo13/estadoHumedad");
  }else if(humedad == 1025){
    send_mqtt_message("???", "/grupo13/humedad");
    send_mqtt_message("Sensor fallando", "/grupo13/estadoHumedad");
  }else{
    int porcentajeHumedad = 100 - map(humedad, 1, 1023, 0, 100);
    String resultado = String(porcentajeHumedad);
    humedad = porcentajeHumedad;

    send_mqtt_message(resultado, "/grupo13/humedad");
    send_mqtt_message("Sensor operando correctamente", "/grupo13/estadoHumedad");
  }

  //Tranformamos la humedad obtenida en un porcentaje deseado
  int porcentajeHumedad = 100 - map(humedad, 1, 1023, 0, 100);
}

// Funcion para realizar la conexion al internet del celular
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Conexión WiFi establecida");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());
}

// Funcion para reconectar con MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("Conectado al servidor MQTT");
      client.subscribe(topico3);
    } else {
      Serial.print("Error, rc=");
      Serial.print(client.state());
      Serial.println(" Intenta nuevamente en 5 segundos");
      delay(5000);
    }
  }
}

// Funcion para enviar los datos requeridos a MQTT
void send_mqtt_message(String message, const char* mqtt_topic) {
  Serial.print("Publicando mensaje en el tópico ");
  Serial.print(mqtt_topic);
  Serial.print(": ");
  Serial.println(message);

  if (client.publish(mqtt_topic, message.c_str())) {
    Serial.println("Mensaje publicado con éxito");
  } else {
    Serial.println("Error al publicar el mensaje");
  }
}
