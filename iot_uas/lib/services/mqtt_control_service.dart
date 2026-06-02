import 'dart:convert';

import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';

class MQTTControlService {
  static const String broker = "broker.emqx.io";
  static const int port = 1883;

  static const String topicBuzzer = "iot7/tandon/buzzer";

  late MqttServerClient client;

  MQTTControlService() {
    client = MqttServerClient(
      broker,
      "flutter_control_${DateTime.now().millisecondsSinceEpoch}",
    );

    client.port = port;
    client.secure = false;
    client.keepAlivePeriod = 20;
    client.autoReconnect = true;
    client.resubscribeOnAutoReconnect = true;
    client.logging(on: true);

    client.onConnected = onConnected;
    client.onDisconnected = onDisconnected;
  }

  Future<void> connect() async {
    try {
      final connMess = MqttConnectMessage()
          .withClientIdentifier(
        "flutter_control_${DateTime.now().millisecondsSinceEpoch}",
      )
          .startClean();

      client.connectionMessage = connMess;

      print("====================");
      print("CONNECTING MQTT CONTROL EMQX");

      await client.connect();

      if (client.connectionStatus?.state == MqttConnectionState.connected) {
        print("MQTT CONTROL EMQX CONNECTED");
      } else {
        print("MQTT CONTROL EMQX FAILED");
        disconnect();
      }
    } catch (e) {
      print("====================");
      print("MQTT CONTROL EMQX ERROR");
      print(e);
      disconnect();
    }
  }

  void publishBuzzer(bool status) {
    if (client.connectionStatus?.state != MqttConnectionState.connected) {
      print("MQTT CONTROL belum connect");
      return;
    }

    final payload = jsonEncode({
      "buzzer": status,
    });

    final builder = MqttClientPayloadBuilder();
    builder.addString(payload);

    client.publishMessage(
      topicBuzzer,
      MqttQos.atMostOnce,
      builder.payload!,
    );

    print("====================");
    print("PUBLISH BUZZER KE EMQX");
    print("TOPIC: $topicBuzzer");
    print("PAYLOAD: $payload");
  }

  void disconnect() {
    client.disconnect();
  }

  void onConnected() {
    print("====================");
    print("MQTT CONTROL EMQX CONNECTED");
  }

  void onDisconnected() {
    print("====================");
    print("MQTT CONTROL EMQX DISCONNECTED");
  }
}