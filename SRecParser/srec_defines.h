#ifndef SREC_DEFINES_H
#define SREC_DEFINES_H

#include <QtCore>


#define SREC_MODULE_LENGTH		20
#define SREC_VERSION_LENGTH		2
#define SREC_REVISION_LENGTH	2
#define SREC_COMMENT_LENGTH		36
#define SREC_DATA_SIZE_MAX		64


//!< Describes the S-Record header (S-Rec type 0).
typedef struct {
	quint32		line_num_;
	QByteArray	module_name_;
	quint16		version_num_;
	quint16		revision_num_;
	QByteArray	comment_;
} srecord_header_t;

//!< Describes an s-record which contains data (S1, S2 & S3).
typedef struct {
	quint32		line_num_;
	quint32		addr_;
	quint32		addr_len_;
	QByteArray	payload_;
	quint8		payload_len_;
	quint8		rec_type_;
	quint8		checksum_calc_;
	quint8		checksum_found_;
} srecord_data_t;



#endif // SREC_DEFINES_H
