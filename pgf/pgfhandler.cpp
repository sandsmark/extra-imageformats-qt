#include <stdint.h>
#include <stdio.h>
#include <QImage>
#include <QImageIOHandler>

#include <libpgf/PGFimage.h>


class PgfHandler: public QImageIOHandler {
public:
  PgfHandler() {}

  bool canRead() const { return canRead(device()); }
  static bool canRead( QIODevice *device );

  bool read( QImage *image );
  QByteArray name() const { return "pgf"; }
};

bool PgfHandler::canRead( QIODevice *device ) {
  return device->peek(3) == "PGF";
}

bool PgfHandler::read( QImage *image ) {
  int w, h;
  bool ret = true;

  QByteArray buf = device()->readAll();
  CPGFMemoryStream pgfMS( (unsigned char*)buf.data(), buf.size() );

  CPGFImage pgf;

  try {
	pgf.Open( &pgfMS );
	pgf.Read() ;
  }
  catch (IOException&) {
	return false;
  }
  
  w = pgf.Width();
  h = pgf.Height();

  *image = *new QImage( w, h, QImage::Format_RGB888 );

  try {
	int map[] = { 2, 1, 0 };
	pgf.GetBitmap( image->bytesPerLine(), image->bits(), 24, map );
  }
  catch (IOException&) {
	delete image;
	*image = QImage();

	return false;
  }

  return ret;
}

class PgfPlugin: public QImageIOPlugin {
public:
  QStringList keys() const { return QStringList() << "pgf"; }

  Capabilities capabilities(QIODevice *device, const QByteArray &format) const;
  QImageIOHandler *create(QIODevice *device, const QByteArray &format = QByteArray()) const;
};

QImageIOPlugin::Capabilities PgfPlugin::capabilities( QIODevice *device, const QByteArray &format ) const {
  if ( !format.isEmpty() && format != "pgf" ) return 0;
  if ( !device || !device->isOpen() ) return 0;

  if ( device->isReadable() && PgfHandler::canRead(device) )
  	return Capabilities(CanRead);

  return 0;
}

QImageIOHandler *PgfPlugin::create( QIODevice *device, const QByteArray &format ) const {
  PgfHandler *handler = new PgfHandler();
  handler->setDevice( device );
  handler->setFormat( format.isEmpty()? "pgf": format );
  return handler;
}

Q_EXPORT_STATIC_PLUGIN(PgfPlugin)
Q_EXPORT_PLUGIN2(pgf, PgfPlugin)
