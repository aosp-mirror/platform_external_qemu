import QtQuick 2.0
import QtQml.RemoteObjects 1.0
import usertypes 1.0

TypeWithModelReplica {
    node: Node {
        registryUrl: "local:testModel"
    }
}
