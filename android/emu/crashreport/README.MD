# Crash reporting module

This module provides functionalities for easily integrating crash reporting into your code. It leverages several key components:

- Crash Uploader Tool: This tool examines crash dumps (with symbol information available) and facilitates their upload to Google.
- Crash Consent Providers: These determine whether a crash should be uploaded, allowing you to choose the suitable provider for your application and link it into your final executable.
- Annotation Tools: This set of classes enables you to create annotations and leave breadcrumbs within your code, which can be used for detailed crash report analysis.
- Hang Detector: This class monitors the application's responsiveness by requiring a heartbeat at regular intervals. If the heartbeat is absent, it triggers a crash to ensure timely problem identification.

This module simplifies the integration of crash reporting, enabling you to effectively capture and analyze crashes, ultimately improving your application's stability and user experience.
