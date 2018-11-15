#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
/* include libmobi header */
#include <mobi.h>

// openssl for md5
#include <openssl/md5.h>

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("Usage: mobichecksum filename.mobi\n");
		exit(1);
	}
	char *filename = argv[1];

	MOBI_RET mobi_ret;

	/* Initialize main MOBIData structure */
	MOBIData *m = mobi_init();
	if (m == NULL) {
		printf("Memory allocation failed\n");
		exit(1);
	}


	errno = 0;
	FILE *file = fopen(filename, "rb");
	if (file == NULL) {
		int errsv = errno;
		printf("Error opening file: %s (%s)\n", filename, strerror(errsv));
		mobi_free(m);
		exit(1);
	}
	/* MOBIData structure will be filled with loaded document data and metadata */
	mobi_ret = mobi_load_file(m, file);
	fclose(file);

	MOBIRawml *rawml = mobi_init_rawml(m);
	if (rawml == NULL) {
		printf("Memory allocation failed\n");
		mobi_free(m);
		exit(1);
	}

	m->mh->uid = 0; // we do not care about UID since we want a checksum

	mobi_ret = mobi_parse_rawml(rawml, m);
	if (mobi_ret != MOBI_SUCCESS) {
		printf("Parsing rawml failed\n");
		mobi_free(m);
		mobi_free_rawml(rawml);
		exit(1);
	}

	MD5_CTX md5;
	MD5_Init(&md5);

	char partname[FILENAME_MAX];
	if (rawml->markup != NULL) {
		/* Linked list of MOBIPart structures in rawml->markup holds main text files */
		MOBIPart *curr = rawml->markup;
		while (curr != NULL) {
			MOBIFileMeta file_meta = mobi_get_filemeta_by_type(curr->type);
//			printf("%s\n", curr->data);
			MD5_Update(&md5, curr->data, curr->size);
			curr = curr->next;
		}
	}
	if (rawml->flow != NULL) {
		/* Linked list of MOBIPart structures in rawml->flow holds supplementary text files */
		MOBIPart *curr = rawml->flow;
		/* skip raw html file */
		curr = curr->next;
		while (curr != NULL) {
			MOBIFileMeta file_meta = mobi_get_filemeta_by_type(curr->type);
//			printf("%s\n", curr->data);
			MD5_Update(&md5, curr->data, curr->size);
			curr = curr->next;
		}
	}
	if (rawml->resources != NULL) {
		/* Linked list of MOBIPart structures in rawml->resources holds binary files, also opf files */
		MOBIPart *curr = rawml->resources;
		/* jpg, gif, png, bmp, font, audio, video, also opf, ncx */
		while (curr != NULL) {
			MOBIFileMeta file_meta = mobi_get_filemeta_by_type(curr->type);
			if (curr->size > 0) {
				// printf("%s\n", curr->data);
				MD5_Update(&md5, curr->data, curr->size);
			}
			curr = curr->next;
		}
	}

	unsigned char digest[16];

	char hex_digest[33];

	MD5_Final(digest, &md5);

	for (int i = 0; i < 16; i++)
		sprintf(&hex_digest[i * 2], "%02x", (unsigned int)digest[i]);

	printf("%s\n", hex_digest);

	mobi_free_rawml(rawml);
	mobi_free(m);
}