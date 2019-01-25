import QtQuick 2.0
import QtQml.RemoteObjects 1.0
import usertypes 1.0

ComplexTypeReplica {
    // test that the existence of this property doesn't cause issues
    property QtObject object: QtObject {}

    node: Node {
        registryUrl: "local:testExtraComplex"
    }
}

