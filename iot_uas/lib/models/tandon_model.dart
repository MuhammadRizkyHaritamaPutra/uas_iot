import 'dart:convert';

class TandonModel {
  final int waterLevel;
  final String statusAir;

  final int ledMerah;
  final int ledKuning;
  final int ledHijau;

  final int buzzer;

  final String? sentAtMs;

  TandonModel({
    required this.waterLevel,
    required this.statusAir,
    required this.ledMerah,
    required this.ledKuning,
    required this.ledHijau,
    required this.buzzer,
    required this.sentAtMs,
  });

  factory TandonModel.fromJson(
      Map<String, dynamic> json,
      ) {
    final conString =
    json["m2m:rsp"]["pc"]["m2m:cin"]["con"];

    final content = jsonDecode(conString);

    return TandonModel(
      waterLevel: content["water_level"] ?? 0,

      statusAir: content["status_air"] ?? "-",

      ledMerah: content["led_merah"] ?? 0,

      ledKuning: content["led_kuning"] ?? 0,

      ledHijau: content["led_hijau"] ?? 0,

      buzzer: content["buzzer"] ?? 0,

      sentAtMs: content["sent_at_ms"]?.toString(),
    );
  }

  int get flutterLatencyMs {
    if (sentAtMs == null) return 0;

    final sentTime = int.tryParse(sentAtMs!);

    if (sentTime == null || sentTime == 0) return 0;

    final receivedTime = DateTime.now().millisecondsSinceEpoch;

    final latency = receivedTime - sentTime;

    if (latency < 0) return 0;

    return latency;
  }
}