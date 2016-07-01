#ifndef SREC_PARSER_H
#define SREC_PARSER_H

#include "srecparser_global.h"
#include "srec_defines.h"

class SRECPARSER_EXPORT SRecParser
{
public:
	SRecParser();
	~SRecParser();

	bool flush();
	bool parseFile(const QString & filename);
	bool parseFile(QFile * file);
	bool parseLine(quint32 line_num, const QString * line);

private:
	bool header(const srecord_header_t *header);
	bool data(const srecord_data_t * data);
	void startAddress(const srecord_data_t *data);
	bool startSegment(quint32 addr);
	bool finishSegment(quint32 addr, quint32 len);

	void parseError(quint32 lineNum, const QString & str);

	bool readByte(QString * str, quint8 *byte, quint32 line_num, const QString &err_str);
	bool readNibble(QString * str, quint8 *b, quint32 line_num, const QString &err_str);
	bool parsedData(const srecord_data_t * srec_data);

	QThread	*thread_;
	bool in_seg_;
	quint32 seg_addr_;
	quint32 seg_len_;
	QByteArray *buffer_;
};

#endif // SREC_PARSER_H
