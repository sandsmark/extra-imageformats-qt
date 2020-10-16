#include <QImageIOPlugin>
#include <QImageIOHandler>

class BpgPlugin: public QImageIOPlugin {
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QImageIOHandlerFactoryInterface" FILE "extensions.json")

public:
  QStringList keys() const { return QStringList() << "bpg"; }
   Capabilities capabilities(QIODevice *device, const QByteArray &format) const;
   QImageIOHandler *create(QIODevice *device, const QByteArray &format = QByteArray()) const;
};
