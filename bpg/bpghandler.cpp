#include <stdint.h>
#include <stdio.h>
#include <QImage>
#include <QImageIOHandler>

extern "C" {
#include <libbpg.h>
}

#include "bpg-decl.h"

class BpgHandler: public QImageIOHandler {
public:
  BpgHandler() {}

  bool canRead() const { return canRead(device()); }
  static bool canRead( QIODevice *device );

  bool read( QImage *image );
  QByteArray name() const { return "bpg"; }
};

bool BpgHandler::canRead( QIODevice *device ) {
  return device->peek(3) == "BPG";
}

static inline uint32_t bgr_to_rgb( uint32_t rgb ) {
  return ((rgb << 16) & 0xff0000) | ((rgb >> 16) & 0xff) | (rgb & 0xff00ff00);
}

bool BpgHandler::read( QImage *image ) {
  BPGImageInfo img_info;
  BPGDecoderContext *img = NULL;
  int w, h;
  bool ret = true;

  QByteArray buf = device()->readAll();

  img = bpg_decoder_open();
  if (bpg_decoder_decode(img, (uint8_t*)buf.data(), buf.size()) < 0) {
  	ret = false;
  	goto EXIT;
  }

  bpg_decoder_get_info(img, &img_info);
  w = img_info.width;
  h = img_info.height;

  *image = *new QImage( w, h, QImage::Format_ARGB32 );

  bpg_decoder_start(img, BPG_OUTPUT_FORMAT_RGBA32);

  for (int y = 0; y < h; ++y) {
  	uint32_t *scanLine = (uint32_t*)image->scanLine(y);
	bpg_decoder_get_line(img, scanLine);
	for ( int x = 0; x < w; x++ )
		scanLine[x] = bgr_to_rgb( scanLine[x] );
  }

EXIT:
  if (img) bpg_decoder_close(img);

  return ret;
}

QImageIOPlugin::Capabilities BpgPlugin::capabilities( QIODevice *device, const QByteArray &format ) const {

  if ( format == "bpg" ) return Capabilities(CanRead);
  if ( !format.isEmpty() || !device->isOpen() ) return 0;

  if ( device->isReadable() && BpgHandler::canRead(device) )
  	return Capabilities(CanRead);

  return 0;
}

QImageIOHandler *BpgPlugin::create( QIODevice *device, const QByteArray &format ) const {
  BpgHandler *handler = new BpgHandler();
  handler->setDevice( device );
  handler->setFormat( format );
  return handler;
}
