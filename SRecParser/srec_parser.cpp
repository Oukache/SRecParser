#include "srec_parser.h"

SRecParser::SRecParser()
	:in_seg_(false), seg_addr_(0), seg_len_(0), buffer_(new QByteArray)
{
}

SRecParser::~SRecParser()
{
	buffer_->clear();
	delete buffer_;
	buffer_ = NULL;
}

bool SRecParser::readByte(QString * str, quint8 *byte, quint32 line_num, const QString &err_str)
{
	quint8 b1, b2;

	if (readNibble(str, &b1, line_num, err_str) && (readNibble(str, &b2, line_num, err_str))) {

		*byte = b1 << 4 | b2;
		return true;
	}
	return false;
}

bool SRecParser::readNibble(QString * str, quint8 *b, quint32 line_num, const QString &err_str)
{
	qint8 ch = str->mid(0,1).at(0).toLatin1(); 
	str->remove(0, 1);

	if ((ch >= '0') && (ch <= '9')) {
		*b = ch - '0';
		return true;
	}

	if ((ch > 'A') && (ch <= 'F')) {
		*b = ch - 'A' + 10;
		return true;
	}

	if ((ch >= 'a') && (ch <= 'f')) {
		*b = ch - 'a' + 10;
		return true;
	}

	parseError(line_num, QString("parsing %s, expecting hex digit, found '%c'").arg(err_str).arg(ch));
	return false;
}

bool SRecParser::parsedData(const srecord_data_t * srec_data)
{
	if (in_seg_ && (srec_data->addr_ != (seg_addr_ + seg_len_))) {
		this->flush();
	}

	if (in_seg_) {

		in_seg_ = true;
		seg_addr_ = srec_data->addr_;
		seg_len_ = 0;

		if (!startSegment(seg_addr_)) {
			return false;
		}
	}

	if (!data(srec_data)) {
		return false;
	}

	seg_len_ += srec_data->payload_len_;
	return true;
}

bool SRecParser::flush()
{
	if (in_seg_) {
		if (!finishSegment(seg_addr_, seg_len_)) {
			return false;
		}
		in_seg_ = false;
	}
	return true;
}

bool SRecParser::parseFile(const QString & filename)
{
	QFile file(filename);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return false;
	}

	bool res = parseFile(&file);
	file.close();

	return res;
}

bool SRecParser::parseFile(QFile * file)
{
	quint32 line_No = 0;
	QString tmp_line;

	QTextStream in(file);
	while (!in.atEnd()) {

		tmp_line = in.readLine();
		line_No++;

		if (!parseLine(line_No, &tmp_line)) {
			return false;
		}
	}
	this->flush();
	return true;
}

bool SRecParser::parseLine(quint32 line_num, const QString * line)
{
	srecord_data_t srec_data;
	srecord_header_t srec_header;

	memset(&srec_data, 0, sizeof(srec_data));
	buffer_->clear();

	if (line->at(0) != 'S') {
		parseError(line_num, "doesn't start with an 'S'");
		return false;
	}

	if (!line->at(1).isDigit()) {
		parseError(line_num, QString("expecting digit (0-9), found: '%c'").arg(line->at(1)));
		return false;
	}

	QString s(line->mid(2));
	quint8 line_len;

	if (!readByte(&s, &line_len, line_num, "count")) {
		return false;
	}

	quint8 checksum_calc = line_len;

	for (int i = 0; i < (line_len - 1); i++) {

		quint8 tmp_byte;
		if (!readByte(&s, &tmp_byte, line_num, "data")) {
			return false;
		}
		checksum_calc += tmp_byte;
		buffer_->append(tmp_byte & 0xFF);
	}

	checksum_calc = ~checksum_calc;

	quint8 checksum_found;

	if (!readByte(&s, &checksum_found, line_num, "checksum")) {
		return false;
	}

	if (checksum_found != checksum_calc) {
		parseError(line_num, QString("found checksum 0x%02x, expecting 0x%02x")
			.arg(checksum_found).arg(checksum_calc));
		return false;
	}

	qint8 srec_digit = line->at(1).toLatin1();
	switch (srec_digit) {

		case '0': {

			memset(&srec_header, 0, sizeof(srec_header));
			srec_header.line_num_ = line_num;
			srec_header.module_name_ = buffer_->mid(2, 20);
			srec_header.version_num_ = buffer_->mid(22, 2).toUShort(0, 16);
			srec_header.revision_num_ = buffer_->mid(24, 2).toUShort(0, 16);
			srec_header.comment_ = buffer_->mid(26, 36);

			this->flush();
			this->header(&srec_header);
			break;
		}

		case '1':
		case '2':
		case '3': {

			memset(&srec_data, 0, sizeof(srec_data));
			srec_data.line_num_ = line_num;
			srec_data.addr_len_ = srec_digit - '1' + 2;
			srec_data.rec_type_ = srec_digit - '0';
			srec_data.checksum_calc_ = checksum_calc;
			srec_data.checksum_found_ = checksum_found;

			quint8 *x = (quint8*)buffer_->data();

			for (int addrIdx = 0; addrIdx < srec_data.addr_len_; addrIdx++) {
				srec_data.addr_ <<= 8;
				srec_data.addr_ += *x++;
			}
			srec_data.payload_len_ = line_len - srec_data.addr_len_ - 1;
			
			memcpy(srec_data.payload_.data(), x, srec_data.payload_len_);
			parsedData(&srec_data);
			break;
		}

		case '5': {

			this->flush();
			break;
		}

		case '7':
		case '8':
		case '9': {

			memset(&srec_data, 0, sizeof(srec_data));
			srec_data.line_num_ = line_num;
			srec_data.addr_len_ = '9' - srec_digit + 2;
			srec_data.rec_type_ = srec_digit - '0';
			srec_data.checksum_calc_ = checksum_calc;
			srec_data.checksum_found_ = checksum_found;

			quint8 *x = (quint8*)buffer_->data();

			for (int addrIdx = 0; addrIdx < srec_data.addr_len_; addrIdx++) {

				srec_data.addr_ <<= 8;
				srec_data.addr_ += *x++;
			}
			this->flush();
			startAddress(&srec_data);
			break;
		}

		default: {
			parseError(line_num, QString("Unrecognized S-Record: S%c").arg(srec_digit));
			return false;
		}
	}

	return true;
}


bool SRecParser::header(const srecord_header_t *header)
{
#ifdef _DEBUG
	qDebug() << "Module: '" << header->module_name_ << "', " << endl;
	qDebug() << "Ver: '" << header->version_num_ << "', " << endl;
	qDebug() << "Rev: '" << header->revision_num_ << "', " << endl;
	qDebug() << "Descr: '" << header->comment_ << "'" << endl;
#endif
	return true;
}

bool SRecParser::data(const srecord_data_t * data)
{
	/*
#ifdef _DEBUG
	for (int i = 0; i < data->payload_len_; i++) {

		if () {

		}
	}
#endif
	*/
	return true;
}

void SRecParser::parseError(quint32 lineNum, const QString & str)
{
#ifdef _DEBUG
	qDebug() << "Error: ";
	if (lineNum > 0) {
		qDebug() << "line " << lineNum << ": ";
	}
	qDebug() << str << endl;
#endif
}

void SRecParser::startAddress(const srecord_data_t *data)
{

}

bool SRecParser::startSegment(quint32 addr)
{
	return true;
}

bool SRecParser::finishSegment(quint32 addr, quint32 len)
{
	return true;
}

