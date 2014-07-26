
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


char	buff[512];

void argerror(char *progname)
{
	fprintf(stderr, "Usage: %s file [-o file]\n", progname);
	exit(EXIT_FAILURE);
}

char *getOFileName(int argc, char **argv)
{
	int	i;

	for (i=1; i<argc-1; i++)
	{
		if (strcmp(argv[i], "-o") == 0) {
			return argv[i+1];
		};
	};
	return NULL;
}

#define IO_FOPEN	"Unable to open file."
#define IO_READ		"Unable to read file."
#define IO_WRITE	"Unable to write to file."

void ioError(const char *err, char *filename)
{
	fprintf(stderr, "File %s: %s\n", filename, err);
}

void outElf(FILE *fo, FILE *fi, int elfPos)
{
	fseek(fi, elfPos, SEEK_SET);

	while (fread(buff, 512, 1, fi) == 1) {
		fwrite(buff, 512, 1, fo);
	};

	while (!feof(fi)) {
		fprintf(fo, "%c", fgetc(fi));
	};
	fflush(fo);
}

int main(int argc, char **argv)
{
	int	i, j, n;
	char 	*ofile, *ifile=argv[1];
	FILE	*fi, *fo;

	if (argc < 2) { argerror(argv[0]); };
	ofile = getOFileName(argc, argv);
	if (ofile == NULL) {
		fo = stdout;
	}
	else
	{
		fo = fopen(ofile, "w");
		if (!fo) { ioError(IO_FOPEN, ofile); };
	};

	fi = fopen(ifile, "r");
	if (!fi) { ioError(IO_FOPEN, ifile); };

	n = 0;
	while (fread(buff, 512, 1, fi) == 1)
	{
		for (j=0; j<512; j++)
		{
			if (n == 4)
			{
				outElf(fo, fi, ftell(fi) - 512 + j - 4);
				exit(EXIT_SUCCESS);
			};
			if (n == 0) {
				if (buff[j] == 0x7F) { n++; continue; };
			};
			if (n == 1) {
				if (buff[j] == 'E') { n++; continue; };
			};
			if (n == 2) {
				if (buff[j] == 'L') { n++; continue; };
			};
			if (n == 3) {
				if (buff[j] == 'F') { n++; continue; };
			};			
			n = 0;
		};
	};

	ioError(IO_READ, ifile);
	exit(EXIT_FAILURE);
}

