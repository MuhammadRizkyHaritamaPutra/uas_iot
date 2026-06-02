import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../models/tandon_model.dart';
import '../services/mqtt_monitoring_service.dart';
import '../services/mqtt_control_service.dart';


// ================= MONITORING ANTARES =================

final mqttServiceProvider =
Provider<MQTTService>((ref) {

  final service = MQTTService();

  service.connect();

  ref.onDispose(() {
    service.disconnect();
  });

  return service;
});

final tandonProvider =
StreamProvider<TandonModel>((ref) {

  final mqtt =
  ref.watch(mqttServiceProvider);

  return mqtt.stream;
});


// ================= CONTROL EMQX =================

final mqttControlProvider =
Provider<MQTTControlService>((ref) {

  final service = MQTTControlService();

  service.connect();

  ref.onDispose(() {
    service.disconnect();
  });

  return service;
});