import 'dart:async';
import 'dart:convert';

import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';

import '../models/tandon_model.dart';

class MQTTService {
  // ================= CONFIG =================

  static const String accessKey =
      "2a5f5a0e1e2f03aa:c131d3a03328c48e";

  static const String broker = "mqtt.antares.id";

  static const int port = 1883;

  static const String topic =
      "/oneM2M/resp/antares-cse/$accessKey/json";

  // ==========================================

  late MqttServerClient client;

  final StreamController<TandonModel> controller =
  StreamController.broadcast();

  Stream<TandonModel> get stream => controller.stream;

  MQTTService() {
    client = MqttServerClient(
      broker,
      "",
    );

    client.port = port;

    client.secure = false;

    client.keepAlivePeriod = 20;

    client.autoReconnect = true;

    client.resubscribeOnAutoReconnect = true;

    client.logging(on: true);

    client.onConnected = onConnected;

    client.onDisconnected = onDisconnected;

    client.onSubscribed = onSubscribed;
  }

  // ================= CONNECT =================

  Future<void> connect() async {
    try {
      final connMess = MqttConnectMessage()
          .withClientIdentifier(
        "flutter_${DateTime.now().millisecondsSinceEpoch}",
      )
          .startClean();

      client.connectionMessage = connMess;

      print("====================");
      print("CONNECTING MQTT");

      await client.connect();

      if (client.connectionStatus!.state ==
          MqttConnectionState.connected) {
        print("MQTT CONNECTED");

        subscribe();
      } else {
        print("MQTT FAILED");

        disconnect();
      }
    } catch (e) {
      print("====================");
      print("MQTT ERROR");
      print(e);

      disconnect();
    }
  }

  // ================= SUBSCRIBE =================

  void subscribe() {
    print("====================");
    print("SUBSCRIBE TOPIC:");
    print(topic);

    client.subscribe(
      topic,
      MqttQos.atMostOnce,
    );

    client.updates!.listen((events) {
      try {
        final recMess =
        events[0].payload as MqttPublishMessage;

        final payload =
        MqttPublishPayload.bytesToStringAsString(
          recMess.payload.message,
        );

        print("====================");
        print("PAYLOAD MASUK:");
        print(payload);

        final json = jsonDecode(payload);

        final data = TandonModel.fromJson(json);

        print("====================");
        print("LATENSI ESP32 KE FLUTTER:");
        print("${data.flutterLatencyMs} ms");

        controller.add(data);
      } catch (e) {
        print("====================");
        print("PARSE ERROR");
        print(e);
      }
    });
  }

  // ================= DISCONNECT =================

  void disconnect() {
    client.disconnect();
  }

  // ================= CALLBACK =================

  void onConnected() {
    print("====================");
    print("CONNECTED");
  }

  void onDisconnected() {
    print("====================");
    print("DISCONNECTED");
  }

  void onSubscribed(String topic) {
    print("====================");
    print("SUBSCRIBED:");
    print(topic);
  }
}