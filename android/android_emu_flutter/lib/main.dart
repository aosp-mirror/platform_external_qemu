import 'package:flutter/material.dart';
import 'dart:ui' as ui;
import 'dart:async';
import 'dart:typed_data';

import 'package:grpc/grpc.dart' hide ConnectionState;
import 'package:android_emu_flutter/src/generated/emulator_controller.pb.dart' as emulator_controller;
import 'package:android_emu_flutter/src/generated/emulator_controller.pbenum.dart';
import 'package:android_emu_flutter/src/generated/emulator_controller.pbgrpc.dart';
import 'package:android_emu_flutter/src/generated/google/protobuf/empty.pb.dart' as google_protobuf_empty;

void main() {
  runApp(MyApp());
}

class MyApp extends StatelessWidget {
  // This widget is the root of your application.
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Flutter Demo',
      theme: ThemeData(
        // This is the theme of your application.
        //
        // Try running your application with "flutter run". You'll see the
        // application has a blue toolbar. Then, without quitting the app, try
        // changing the primarySwatch below to Colors.green and then invoke
        // "hot reload" (press "r" in the console where you ran "flutter run",
        // or simply save your changes to "hot reload" in a Flutter IDE).
        // Notice that the counter didn't reset back to zero; the application
        // is not restarted.
        primarySwatch: Colors.blue,
      ),
      home: MyHomePage(title: 'Flutter Demo Home Page'),
    );
  }
}

class MyHomePage extends StatefulWidget {
  MyHomePage({Key key, this.title}) : super(key: key);

  // This widget is the home page of your application. It is stateful, meaning
  // that it has a State object (defined below) that contains fields that affect
  // how it looks.

  // This class is the configuration for the state. It holds the values (in this
  // case the title) provided by the parent (in this case the App widget) and
  // used by the build method of the State. Fields in a Widget subclass are
  // always marked "final".

  final String title;

  @override
  _MyHomePageState createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  Client client = new Client();
  final Size _size = new Size(576, 1024);
  void initState() {
    super.initState();
  }

  @override
  Widget build(BuildContext context) {
    return new Scaffold(
      appBar: new AppBar(
        title: new Text(widget.title),
      ),
      body: new Container(
        child: StreamBuilder<ui.Image>(
          stream: client.generateScreenshot(_size),
          builder: (BuildContext context, AsyncSnapshot<ui.Image> snapshot) {
              if (snapshot.hasError) {
                print('##### streamScreenshot has error:');
                new Center(child: new Text('Error'));
              } else if (snapshot.connectionState == ConnectionState.active) {
                var result = snapshot.data;
                return new CustomPaint(
                  painter: new MyPainter(image: result),
                );
              } else {
                return new Center(child: new Text('loading'));
              }
          } 
        ),
      ),
    );
  }
}

class MyPainter extends CustomPainter {
  ui.Image image;
  MyPainter({this.image,});

  @override
  void paint(Canvas canvas, Size size) {
    canvas.drawImage(image, new Offset(0.0, 0.0), new Paint());
  }

  @override
  bool shouldRepaint(CustomPainter oldDelegate) {
    return true;
  }

}

class Client {
  EmulatorControllerClient stub;
  final timeoutInHours = 5;
  Client() {
    final channel = ClientChannel('0.0.0.0',
        port: 12345,
        options:
            const ChannelOptions(credentials: ChannelCredentials.insecure()));
    stub = EmulatorControllerClient(channel,
        options: CallOptions(timeout: Duration(hours: timeoutInHours)));
  }

  /// Run the getFeature demo. Calls getFeature with a point known to have a
  /// feature and a point known not to have a feature.
  Future<void> getVmState() async {
    await stub.getVmState(google_protobuf_empty.Empty.getDefault());
  }

  Stream<ui.Image> generateScreenshot(Size size) async* {
    final int w = size.width.round();
    final int h = size.height.round();
    final request = new ImageFormat(
        format: ImageFormat_ImgFormat.RGBA8888, 
        width: w,
        height: h,
        display: 0);
    try {
      await for (var img in stub.streamScreenshot(request)) {
        final buffer = await ui.ImmutableBuffer.fromUint8List(img.image);
        final descriptor = ui.ImageDescriptor.raw(
          buffer,
          width: w,
          height: h,
          pixelFormat: ui.PixelFormat.rgba8888,
        );    
        final codec = await descriptor.instantiateCodec(
          targetWidth: w,
          targetHeight: h,
        );
        final frameInfo = await codec.getNextFrame();
        yield frameInfo.image;
      } 
    } catch (e) {
      print('${e}');
      throw new Exception('grpc stream error!');
    }
  }
}
