# Progress Meter C

A terminal-based C take on the Progress Meter app. Add milestones under categories such as Academic, Career, or Personal; assign low, normal, or high priority; set optional due dates and notes; mark items complete; pause and resume work; edit or delete milestones; and search by name or category. Focus views filter open, paused, completed, or high-priority work. The dashboard shows progress bars and deadline alerts.

## Build and run

```sh
make
./progress_meter
```

Or compile directly with `cc -std=c11 -Wall -Wextra -Wpedantic -O2 progress_meter.c -o progress_meter`.

Optional due dates use `YYYY-MM-DD`. The app saves to `progress_data.txt` in the current working directory. Existing save files from older versions remain readable; the new fields are added when the app next saves.

This is a deliberately minimal terminal version. It does not include the original Flutter app's AI assessments, integrations, or mobile interface.
