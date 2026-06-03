import 'dart:convert';

class TandonModel {
  final int waterLevel;
  final String statusAir;

  final int ledMerah;
  final int ledKuning;
  final int ledHijau;

  final int buzzer;

  final String? sentAtMs;

  final int receivedAtMs;

  TandonModel({
    required this.waterLevel,
    required this.statusAir,
    required this.ledMerah,
    required this.ledKuning,
    required this.ledHijau,
    required this.buzzer,
    required this.sentAtMs,
    required this.receivedAtMs,
  });

  factory TandonModel.fromJson(
      Map<String, dynamic> json,
      ) {
    final receivedAtMs = DateTime.now().millisecondsSinceEpoch;

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
      receivedAtMs: receivedAtMs,
    );
  }

  int get flutterLatencyMs {
    if (sentAtMs == null) return 0;

    final sentTime = int.tryParse(sentAtMs!);

    if (sentTime == null || sentTime == 0) return 0;

    final latency = receivedAtMs - sentTime;

    // Kalau negatif kecil, anggap karena beda clock ESP32 dan HP.
    // Jangan langsung return 0 terus.
    if (latency < 0 && latency >= -300) {
      return latency.abs();
    }

    // Kalau negatif besar, berarti timestamp memang bermasalah.
    if (latency < -300) {
      return 0;
    }

    return latency;
  }

  String get latencyStatus {
    final latency = flutterLatencyMs;

    if (latency <= 1000) {
      return "Cepat";
    } else if (latency <= 3000) {
      return "Cukup";
    } else {
      return "Lambat";
    }
  }
}
