SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

cd $SCRIPTPATH

print_help() {
    echo "Flags:"
    echo "    --no-snapshot-load: run the emulator without loading the snapshot."
    echo "    --help: print this message."
}

SNAPSHOT_LOAD=true

while [[ $# -gt 0 ]]; do
      KEY="$1"

      case $KEY in
        --no-snapshot-load)
            SNAPSHOT_LOAD=false
            shift
            ;;
        --help)
            print_help
            exit 0
            ;;
        *)
            echo "Unknown option: $KEY"
            print_help
            exit 1
            ;;
      esac
done

EMULATOR_DIR=$ANDROID_SDK_ROOT/emulator
EMULATOR_PATH=$EMULATOR_DIR/emulator
MKSDCARD_PATH=$EMULATOR_DIR/mksdcard

ANDROID_AVD_HOME=$SCRIPTPATH

AVD_ID=$(awk -F "=" '/avdId/ {print $2}' exported.ini | tr -d ' ')
AVD_FOLDER=$AVD_ID.avd
SNAPSHOT_NAME=$(awk -F "=" '/snasphotName/ {print $2}' exported.ini | tr -d ' ')
TARGET=$(awk -F "=" '/target/ {print $2}' exported.ini | tr -d ' ')
SNAPSHOT_PATH=$AVD_FOLDER/snapshots/$SNAPSHOT_NAME

echo $SNAPSHOT_NAME
echo $SNAPSHOT_PATH

if [[ -f "hardware.ini" ]]; then
    HARDWARE_INI="hardware.ini"
elif [[ -f "$AVD_FOLDER/hardware-qemu.ini" ]]; then
    HARDWARE_INI="$AVD_FOLDER/hardware-qemu.ini"
else
    echo "Cannot find hardware.ini"
    exit 1
fi

if [[ -f "config.ini" ]]; then
    CONFIG_INI="config.ini"
elif [[ -f "$AVD_FOLDER/config.ini" ]]; then
    CONFIG_INI="$AVD_FOLDER/config.ini"
else
    echo "Cannot find config.ini"
    exit 1
fi

AVD_INI=$AVD_ID.ini

rm $AVD_INI
echo "avd.ini.encoding=UTF-8
path=$ANDROID_AVD_HOME/$AVD_FOLDER
path.rel=$AVD_FOLDER.avd
target=$TARGET" > $AVD_INI

OLD_AVD_HOME=$(awk -F "=" '/android.avd.home/ {print $2}' $HARDWARE_INI | tr -d ' ')
OLD_ANDROID_SDK_ROOT=$(awk -F "=" '/android.sdk.root/ {print $2}' $HARDWARE_INI | tr -d ' ')
SD_CARD_SIZE=$(awk -F "=" '/sdcard.size/ {print $2}' $CONFIG_INI | tr -d ' ')

mkdir $AVD_FOLDER
mkdir $AVD_FOLDER/snapshots
mkdir $SNAPSHOT_PATH
mv hardware.ini $AVD_FOLDER/hardware-qemu.ini
mv config.ini $AVD_FOLDER/
mv *.qcow2 $SNAPSHOT_PATH
mv ram.* $SNAPSHOT_PATH
mv screenshot.png $SNAPSHOT_PATH
mv snapshot.pb $SNAPSHOT_PATH
mv textures.bin $SNAPSHOT_PATH

# Fix paths

sed -i "s:$OLD_AVD_HOME:$ANDROID_AVD_HOME:g" $AVD_FOLDER/config.ini
sed -i "s:$OLD_AVD_HOME:$ANDROID_AVD_HOME:g" $AVD_FOLDER/hardware-qemu.ini
sed -i "s:$OLD_ANDROID_SDK_ROOT:$ANDROID_SDK_ROOT:g" $AVD_FOLDER/hardware-qemu.ini

cp $AVD_FOLDER/hardware-qemu.ini $SNAPSHOT_PATH

if [ "$SNAPSHOT_LOAD" = true ] ; then
    SNAPSHOT_LOAD_PARAMETER="-qemu -loadvm $SNAPSHOT_NAME"
else
    SNAPSHOT_LOAD_PARAMETER=-no-snapshot-load
fi

# Create SD card
if [[ ! -f "$AVD_FOLDER/sdcard.img" ]]; then
    $MKSDCARD_PATH $SD_CARD_SIZE $AVD_FOLDER/sdcard.img
fi

# Run the emulator
ANDROID_AVD_HOME=$ANDROID_AVD_HOME $EMULATOR_PATH -avd $AVD_ID -feature AllowSnapshotMigration $SNAPSHOT_LOAD_PARAMETER