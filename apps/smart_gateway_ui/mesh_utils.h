#ifndef MESH_UTILS_H
#define MESH_UTILS_H

#include <QString>

namespace MeshUtils {

inline int unicastFromNodeId(const QString& nodeId) {
    if (nodeId.startsWith(QLatin1String("node_"))) {
        bool ok = false;
        int v = nodeId.mid(5).toInt(&ok, 16);
        if (ok) {
            return v;
        }
    }
    return 0;
}

} 

#endif