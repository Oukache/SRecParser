
#include <QtCore/QCoreApplication>
#include <SRecParser\srec_parser.h>

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	if (argc != 2) {
		fprintf(stderr, "Usage: srecinfo srec-file\n");
		exit(1);
	}

	SRecParser parser;

	parser.parseFile(QString(argv[1]));
	parser.flush();

	return a.exec();
}
