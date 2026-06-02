import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../providers/mqtt_provider.dart';

class HomePage extends ConsumerWidget {
  const HomePage({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final state = ref.watch(tandonProvider);
    final mqttControl = ref.read(mqttControlProvider);

    return Scaffold(
      backgroundColor: const Color(0xffF4F4F4),
      body: SafeArea(
        child: state.when(
          data: (data) {
            final bool buzzerOn = data.buzzer == 1;

            final double progressValue =
                data.waterLevel.clamp(0, 100).toDouble() / 100;

            return SingleChildScrollView(
              padding: const EdgeInsets.all(20),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  // ================= HEADER =================

                  Row(
                    mainAxisAlignment: MainAxisAlignment.spaceBetween,
                    children: [
                      CircleAvatar(
                        backgroundColor: Colors.white,
                        child: IconButton(
                          onPressed: () {},
                          icon: const Icon(
                            Icons.arrow_back_ios_new,
                            size: 18,
                          ),
                        ),
                      ),
                      const Text(
                        "Smart Tandon",
                        style: TextStyle(
                          fontSize: 22,
                          fontWeight: FontWeight.bold,
                        ),
                      ),
                      CircleAvatar(
                        backgroundColor: Colors.white,
                        child: IconButton(
                          onPressed: () {},
                          icon: const Icon(Icons.more_vert),
                        ),
                      ),
                    ],
                  ),

                  const SizedBox(height: 40),

                  // ================= WATER LEVEL =================

                  Center(
                    child: Column(
                      children: [
                        const Text(
                          "Water Controller",
                          style: TextStyle(
                            fontSize: 18,
                            fontWeight: FontWeight.w600,
                          ),
                        ),

                        const SizedBox(height: 25),

                        SizedBox(
                          width: 240,
                          height: 240,
                          child: Stack(
                            alignment: Alignment.center,
                            children: [
                              SizedBox(
                                width: 220,
                                height: 220,
                                child: CircularProgressIndicator(
                                  value: progressValue,
                                  strokeWidth: 12,
                                  backgroundColor: Colors.grey.shade300,
                                ),
                              ),

                              Column(
                                mainAxisAlignment: MainAxisAlignment.center,
                                children: [
                                  Text(
                                    "${data.waterLevel}%",
                                    style: const TextStyle(
                                      fontSize: 42,
                                      fontWeight: FontWeight.bold,
                                    ),
                                  ),

                                  const SizedBox(height: 8),

                                  const Text(
                                    "Water Level",
                                    style: TextStyle(
                                      fontSize: 18,
                                    ),
                                  ),
                                ],
                              ),
                            ],
                          ),
                        ),
                      ],
                    ),
                  ),

                  const SizedBox(height: 40),

                  // ================= STATUS ICON =================

                  Row(
                    mainAxisAlignment: MainAxisAlignment.spaceBetween,
                    children: [
                      buildControlButton(
                        icon: Icons.water_drop,
                        label: "Low",
                        active: data.ledMerah == 1,
                      ),

                      buildControlButton(
                        icon: Icons.water,
                        label: "Medium",
                        active: data.ledKuning == 1,
                      ),

                      buildControlButton(
                        icon: Icons.check_circle,
                        label: "Full",
                        active: data.ledHijau == 1,
                      ),
                    ],
                  ),

                  const SizedBox(height: 40),

                  // ================= BUZZER =================

                  Container(
                    padding: const EdgeInsets.all(20),
                    decoration: BoxDecoration(
                      color: Colors.white,
                      borderRadius: BorderRadius.circular(24),
                    ),
                    child: Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: [
                        const Row(
                          children: [
                            CircleAvatar(
                              backgroundColor: Colors.black,
                              child: Icon(
                                Icons.volume_up,
                                color: Colors.white,
                              ),
                            ),

                            SizedBox(width: 14),

                            Text(
                              "Buzzer Control",
                              style: TextStyle(
                                fontSize: 18,
                                fontWeight: FontWeight.bold,
                              ),
                            ),
                          ],
                        ),

                        Switch(
                          value: buzzerOn,
                          onChanged: (value) {
                            mqttControl.publishBuzzer(value);
                          },
                        ),
                      ],
                    ),
                  ),

                  const SizedBox(height: 40),

                  // ================= TANK INFO =================

                  Container(
                    padding: const EdgeInsets.all(20),
                    decoration: BoxDecoration(
                      color: Colors.white,
                      borderRadius: BorderRadius.circular(24),
                    ),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        const Text(
                          "Tank Information",
                          style: TextStyle(
                            fontSize: 20,
                            fontWeight: FontWeight.bold,
                          ),
                        ),

                        const SizedBox(height: 20),

                        Row(
                          mainAxisAlignment: MainAxisAlignment.spaceBetween,
                          children: [
                            infoItem(
                              Icons.water_drop,
                              "Water",
                              "${data.waterLevel}%",
                            ),

                            infoItem(
                              Icons.volume_up,
                              "Buzzer",
                              buzzerOn ? "ON" : "OFF",
                            ),
                          ],
                        ),

                        const SizedBox(height: 20),

                        Row(
                          mainAxisAlignment: MainAxisAlignment.spaceBetween,
                          children: [
                            infoItem(
                              Icons.lightbulb,
                              "LED 1",
                              data.ledMerah == 1 ? "ON" : "OFF",
                            ),

                            infoItem(
                              Icons.lightbulb,
                              "LED 2",
                              data.ledKuning == 1 ? "ON" : "OFF",
                            ),

                            infoItem(
                              Icons.lightbulb,
                              "LED 3",
                              data.ledHijau == 1 ? "ON" : "OFF",
                            ),
                          ],
                        ),
                      ],
                    ),
                  ),

                  const SizedBox(height: 40),

                  // ================= BUTTON =================

                  SizedBox(
                    width: double.infinity,
                    height: 60,
                    child: ElevatedButton(
                      style: ElevatedButton.styleFrom(
                        backgroundColor: Colors.black,
                        shape: RoundedRectangleBorder(
                          borderRadius: BorderRadius.circular(20),
                        ),
                      ),
                      onPressed: () {},
                      child: const Text(
                        "Realtime Monitoring",
                        style: TextStyle(
                          fontSize: 18,
                          color: Colors.white,
                          fontWeight: FontWeight.bold,
                        ),
                      ),
                    ),
                  ),
                ],
              ),
            );
          },

          loading: () {
            return const Center(
              child: CircularProgressIndicator(),
            );
          },

          error: (e, s) {
            return Center(
              child: Text(e.toString()),
            );
          },
        ),
      ),
    );
  }

  // ================= STATUS BUTTON =================

  Widget buildControlButton({
    required IconData icon,
    required String label,
    required bool active,
  }) {
    return Column(
      children: [
        Container(
          width: 90,
          height: 90,
          decoration: BoxDecoration(
            color: active ? Colors.black : Colors.white,
            borderRadius: BorderRadius.circular(26),
          ),
          child: Icon(
            icon,
            size: 38,
            color: active ? Colors.white : Colors.black,
          ),
        ),

        const SizedBox(height: 10),

        Text(
          label,
          style: const TextStyle(
            fontWeight: FontWeight.w600,
          ),
        ),
      ],
    );
  }

  // ================= INFO ITEM =================

  Widget infoItem(
      IconData icon,
      String title,
      String value,
      ) {
    return Row(
      children: [
        CircleAvatar(
          backgroundColor: Colors.black,
          child: Icon(
            icon,
            color: Colors.white,
          ),
        ),

        const SizedBox(width: 12),

        Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              value,
              style: const TextStyle(
                fontSize: 20,
                fontWeight: FontWeight.bold,
              ),
            ),

            Text(title),
          ],
        ),
      ],
    );
  }
}