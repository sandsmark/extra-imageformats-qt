#include <stdint.h>
#include <stdio.h>
#include <QImage>
#include <QImageIOHandler>

#include <libpgf/PGFimage.h>

#include "pgf-decl.h"

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
	puts("PGFHandler read error");
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

	puts("PGFHandler decode error");
	return false;
  }

  return ret;
}

QImageIOPlugin::Capabilities PgfPlugin::capabilities( QIODevice *device, const QByteArray &format ) const {
  if ( format == "pgf" ) return Capabilities(CanRead);
  if ( !format.isEmpty() || !device->isOpen() ) return 0;

  if ( device->isReadable() && PgfHandler::canRead(device) )
  	return Capabilities(CanRead);

  return 0;
}

QImageIOHandler *PgfPlugin::create( QIODevice *device, const QByteArray &format ) const {
  PgfHandler *handler = new PgfHandler();
  handler->setDevice( device );
  handler->setFormat( format );
  return handler;
}
