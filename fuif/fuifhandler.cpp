#include <stdint.h>
#include <stdio.h>
#include <QImage>
#include <QImageIOHandler>

#include <encoding/encoding.h>


class FuifHandler: public QImageIOHandler {
public:
  FuifHandler() {}

  bool canRead() const { return canRead(device()); }
  static bool canRead( QIODevice *device );

  bool read( QImage *image );
  QByteArray name() const { return "fuif"; }
};

bool FuifHandler::canRead( QIODevice *device ) {
  return device->peek(4) == "FUIF";
}

bool FuifHandler::read( QImage *image ) {
  Image img;
  int w, h;

  QByteArray buf = device()->readAll();

  FILE *f = fmemopen( buf.data(), buf.size(), "rb" );
  FileIO fio( f, "<filename>" );

  if (!fuif_decode(fio, img, default_fuif_options)) return false;

  img.undo_transforms();

  w = img.w;
  h = img.h / img.nb_frames;

  *image = *new QImage( w, h, QImage::Format_ARGB32 );

  for (int y = 0; y < h; ++y) {
  	uint32_t *scanLine = (uint32_t*)image->scanLine(y);
	for ( int x = 0; x < w; x++ )
		scanLine[x] = qRgba(
			img.channel[0].value(y, x),
			img.channel[1].value(y, x),
			img.channel[2].value(y, x),
			(img.nb_channels > 3)? img.channel[3].value(y, x): 255 );
  }

  return true;
}

class FuifPlugin: public QImageIOPlugin {
public:
  QStringList keys() const { return QStringList() << "fuif"; }

  Capabilities capabilities(QIODevice *device, const QByteArray &format) const;
  QImageIOHandler *create(QIODevice *device, const QByteArray &format = QByteArray()) const;
};

QImageIOPlugin::Capabilities FuifPlugin::capabilities( QIODevice *device, const QByteArray &format ) const {

  if ( format == "fuif" ) return Capabilities(CanRead);
  if ( !format.isEmpty() || !device->isOpen() ) return 0;

  if ( device->isReadable() && FuifHandler::canRead(device) )
  	return Capabilities(CanRead);

  return 0;
}

QImageIOHandler *FuifPlugin::create( QIODevice *device, const QByteArray &format ) const {
  FuifHandler *handler = new FuifHandler();
  handler->setDevice( device );
  handler->setFormat( format );
  return handler;
}
