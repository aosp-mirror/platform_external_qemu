import 'package:flutter/material.dart';
import 'dart:ui' as ui;
import 'dart:async';
import 'dart:isolate';
import 'dart:typed_data';
import 'dart:ffi';
import 'dart:io';
import 'package:path/path.dart' as p;

import 'package:grpc/grpc.dart' hide ConnectionState;
import 'package:android_emu_flutter/src/generated/emulator_controller.pb.dart' as emulator_controller;
import 'package:android_emu_flutter/src/generated/emulator_controller.pbenum.dart';
import 'package:android_emu_flutter/src/generated/emulator_controller.pbgrpc.dart';
import 'package:android_emu_flutter/src/generated/google/protobuf/empty.pb.dart' as google_protobuf_empty;
import 'package:ffi/ffi.dart';

class Stat extends Struct {
  @Int64()
  int ignored1;
  @Int64()
  int ignored2;
  @Int64()
  int ignored3;
  @Int64()
  int ignored4;
  @Int64()
  int ignored5;
  @Int64()
  int ignored6;
  @Int64()
  int st_size;
  @Int64()
  int ignored7;
  @Int64()
  int ignored8;
  @Int64()
  int ignored9;
  @Int64()
  int ignored10;
  @Int64()
  int ignored11;
  @Int64()
  int ignored12;
  @Int64()
  int ignored13;
  @Int64()
  int ignored14;
  @Int64()
  int ignored15;
  @Int64()
  int ignored16;
  @Int64()
  int ignored17;
}
 
// int open(const char *path, int oflag, ...);
typedef OpenNative = IntPtr Function(Pointer<Utf8> filename, IntPtr flags, IntPtr mode);
typedef Open = int Function(Pointer<Utf8> filename, int flags, int mode);
final open = processSymbols.lookupFunction<OpenNative, Open>('open');
 
// int __fxstat(int version, int fd, struct stat *buf);
typedef FStatNative = IntPtr Function(
    IntPtr vers, IntPtr fd, Pointer<Stat> stat);
typedef FStat = int Function(int vers, int fd, Pointer<Stat> stat);
final fstat = processSymbols.lookupFunction<FStatNative, FStat>('__fxstat');
 
// int close(int fd);
typedef CloseNative = IntPtr Function(IntPtr fd);
typedef Close = int Function(int fd);
final close = processSymbols.lookupFunction<CloseNative, Close>('close');
 
// void* mmap(void* addr, size_t length,
//            int prot, int flags,
//            int fd, off_t offset)
typedef MMapNative = Pointer<Uint8> Function(Pointer<Uint8> address, IntPtr len,
    IntPtr prot, IntPtr flags, IntPtr fd, IntPtr offset);
typedef MMap = Pointer<Uint8> Function(
    Pointer<Uint8> address, int len, int prot, int flags, int fd, int offset);
final mmap = processSymbols.lookupFunction<MMapNative, MMap>('mmap');
 
// int munmap(void *addr, size_t length)
typedef MUnMapNative = IntPtr Function(Pointer<Uint8> address, IntPtr len);
typedef MUnMap = int Function(Pointer<Uint8> address, int len);
final munmap = processSymbols.lookupFunction<MUnMapNative, MUnMap>('munmap');

// int ftruncate(int fd, off_t length);
typedef FTruncateNative = IntPtr Function(IntPtr fd, IntPtr offset);
typedef FTruncate = int Function(int fd, int offset);
final ftruncate = processSymbols.lookupFunction<FTruncateNative, FTruncate>('ftruncate');


final processSymbols = DynamicLibrary.process();
 
const int kPageSize = 4096;
const int kProtRead = 1;
const int kProtWrite = 2;
const int kMapPrivate = 2;
const int kMapFailed = -1;
const int kMapFixed = 16;
final int kMapAnonymous = Platform.isMacOS ? 0x1000 : 0x20;

final MMAP_FILE = "/var/tmp/emu-client-12345.mmap";

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
          stream: fetchAndConvert(_size),
          builder: (BuildContext context, AsyncSnapshot<ui.Image> snapshot) {
              if (snapshot.hasError) {
                print('##### streamScreenshot has error:');
                new Center(child: new Text('Error'));
              } else if (snapshot.connectionState == ConnectionState.active) {
                final result = snapshot.data;
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

Uint8List getResultHelper(Uint8List bytes) {
    return bytes;
}

// Spawn an isolate to fetch screenshot
Stream<ui.Image> fetchAndConvert(Size size) async* {
  int w = size.width.round();
  int h = size.height.round();
  final length = w * h * 4;
  var receivePort = new ReceivePort();
  final filename = MMAP_FILE;
  final cfilename = filename.toNativeUtf8();
  final int fd = open(cfilename, 0100|02, 0666);
  malloc.free(cfilename);
  if (fd == 0) throw 'failed to open';
  final lengthRoundedUp = (length + kPageSize - 1) & ~(kPageSize - 1);
  // truncate the file to specified length
  if (ftruncate(fd, lengthRoundedUp) == -1) {
    throw 'failed to truncate to length ${lengthRoundedUp}';
  }
  await Isolate.spawn(fetchImage, receivePort.sendPort);
  await for (var data in receivePort) {
    
    final result = mmap(nullptr, lengthRoundedUp, kProtRead, kMapPrivate, fd, 0);
    if (result.address == kMapFailed) throw 'failed to map';
    Uint8List bytes = result.cast<Uint8>().asTypedList(length);
    
    final buffer = await ui.ImmutableBuffer.fromUint8List(bytes);
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
    munmap(result, lengthRoundedUp);
  }
  close(fd);

}

void fetchImage(SendPort port) async {
  //print('starting in new isolate!');
  final int w = 576;
  final int h = 1024;
  final timeoutInHours = 5;
  int lastUpdatedTs = 0;
  final channel = ClientChannel('0.0.0.0',
        port: 12345,
        options:
            const ChannelOptions(credentials: ChannelCredentials.insecure()));
  final stub = EmulatorControllerClient(channel,
        options: CallOptions(timeout: Duration(hours: timeoutInHours)));
  final t = new ImageTransport(
      channel: ImageTransport_TransportChannel.MMAP,
      handle: 'file:///' + MMAP_FILE);
  final request = new ImageFormat(
      format: ImageFormat_ImgFormat.RGBA8888, 
      width: w,
      height: h,
      display: 0,
      transport: t);
    
  await for (var resp in stub.streamScreenshot(request)) {
      int currentTime = DateTime.now().millisecondsSinceEpoch;
      int diff = currentTime - (resp.timestampUs ~/ 1000).toInt();
      print('###### latency: ${diff}');
      // Since, we read from MMAP_FILE, 
      // only signal the main ui isolate that new image has arrived
      port.send(resp.seq);
      //final source = Uint8List.fromList(resp.image);
      //port.send(TransferableTypedData.fromList([source]));
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

