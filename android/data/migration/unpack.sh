EMULATOR_PATH=$ANDROID_SDK_ROOT/emulator/emulator

SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

cd $SCRIPTPATH

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
elif [[ -f "$AVD_FOLDER/hardware.ini" ]]; then
    HARDWARE_INI="$AVD_FOLDER/hardware.ini"
else
    echo "Cannot find hardware.ini"
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

mkdir $AVD_FOLDER
mkdir $AVD_FOLDER/snapshots
mkdir $SNAPSHOT_PATH
mv hardware.ini $AVD_FOLDER/
mv config.ini $AVD_FOLDER/
mv *.qcow2 $AVD_FOLDER/
mv ram.* $SNAPSHOT_PATH
mv screenshot.png $SNAPSHOT_PATH
mv snapshot.pb $SNAPSHOT_PATH
mv textures.bin $SNAPSHOT_PATH

# Fix paths

sed -i "s:$OLD_AVD_HOME:$ANDROID_AVD_HOME:g" $AVD_FOLDER/config.ini
sed -i "s:$OLD_AVD_HOME:$ANDROID_AVD_HOME:g" $AVD_FOLDER/hardware.ini
sed -i "s:$OLD_ANDROID_SDK_ROOT:$ANDROID_SDK_ROOT:g" $AVD_FOLDER/hardware.ini

cp $AVD_FOLDER/hardware.ini $SNAPSHOT_PATH

# Run the emulator
ANDROID_AVD_HOME=$ANDROID_AVD_HOME $EMULATOR_PATH -avd $AVD_ID -verbose