#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

/* #define MAX_DATA 512 */
/* #define MAX_ROWS 100 */

struct Address {
		unsigned int id;
		int set;
		char *name;
		char *email;
};

struct Database {
		unsigned int max_data;
		unsigned max_row;
		struct Address *rows;
};

struct Connection {
		FILE *file;
		struct Database *db;
};

void die(struct Connection *conn, const char *message)
{
		if (errno) {
				perror(message);
		} else {
				printf("ERROR: %s\n", message);
		}

		if (conn) {
				if (conn->file)
						fclose(conn->file);
				if (conn->db)
						free(conn->db);
				free(conn);
		}

		exit(1);
}

void Address_print(struct Address *addr)
{
		printf("%d %s %s\n", addr->id, addr->name, addr->email);
}

void Database_load(struct Connection *conn)
{
		int rc = fread(conn->db, sizeof(struct Database), 1, conn->file);
		if (rc != 1)
				die(conn, "Failed to load database");
}

struct Connection *Database_open(const char *filename, char mode)
{
		struct Connection *conn = malloc(sizeof(struct Connection));
		if (!conn)
				die(conn, "Memory error");

		conn->db = malloc(sizeof(struct Database));
		if (!conn->db)
				die(conn, "Memory error");

		if (mode == 'c') {
				conn->file = fopen(filename, "w");
		} else {
				conn->file = fopen(filename, "r+");

				if (conn->file) {
						Database_load(conn);
				}
		}

		if (!conn->file)
				die(conn, "Failed to open the file");

		return conn;
}


void Database_close(struct Connection *conn)
{
		if (conn) {
				if (conn->file)
						fclose(conn->file);
				if (conn->db)
						free(conn->db);
				free(conn);
		}
}

void Database_write(struct Connection *conn)
{
		rewind(conn->file);

		int rc = fwrite(conn->db, sizeof(struct Database), 1, conn->file);
		if (rc != 1)
				die(conn, "Failed to write database");

		rc = fflush(conn->file);
		if (rc == -1)
				die(conn, "Cannot flush database");
}

void Database_create(struct Connection *conn, unsigned int max_data, unsigned int max_row)
{
		unsigned int i = 0;

		conn->db->max_data = max_data;
		conn->db->max_row = max_row;

		conn->db->rows  = (struct Address*) malloc(sizeof(struct Address) * max_row);
		for (i = 0; i < max_row; i++) {
				struct Address addr = {.id = i, .set = 0 };
				conn->db->rows[i] = addr;
		}
}

void Database_set(struct Connection *conn, int id, const char *name, const char *email) 
{
		struct Address *addr = &conn->db->rows[id];
		unsigned int max_data = conn->db->max_data;
		if (addr->set)
				die(conn, "Already set, delete it first");

		addr->set  = 1;
		char *res = strncpy(addr->name, name, max_data);
		if (!res)
				die(conn, "Name copy failed");
		addr->name[max_data - 1] = '\0';

		res = strncpy(addr->email, email, max_data);
		if (!res)
				die(conn, "Email copy failed");
		addr->email[max_data - 1] = '\0';
}		

void Database_get(struct Connection *conn, int id)
{
		struct Address *addr = &conn->db->rows[id];

		if (addr->set) {
				Address_print(addr);
		} else {
				die(conn, "ID is not set");
		}
}

void Database_delete(struct Connection *conn, unsigned int id)
{
		struct Address addr = {.id = id, .set = 0 };
		conn->db->rows[id] = addr;
}

void Database_list(struct Connection *conn)
{
		unsigned int i = 0;
		struct Database *db = conn->db;

		for (i = 0; i < db->max_row; i++) {
				struct Address *cur = &db->rows[i];

				if (cur->set) {
						Address_print(cur);
				}
		}
}

int main(int argc, char *argv[])
{
		if (argc < 3)
				die(NULL, "USAGE: ex17 <dbfile> <action> [action params]");

		char *filename = argv[1];
		char action = argv[2][0];
		struct Connection *conn = Database_open(filename, action);
		int id = 0;
		int max_data = 0;
		int max_row = 0;

		/* if (argc > 3) id = atoi(argv[3]); */
		/* if (id >= MAX_ROWS) die(conn, "There's not that many records."); */

		switch(action) {
				case 'c':
						if (argc != 5)
								die(conn, "Need max data, max row to set");

						max_data = atoi(argv[3]);
						max_row = atoi(argv[4]);

						Database_create(conn, max_data, max_row);
						Database_write(conn);
						break;

				case 'g':
						if (argc != 4)
								die(conn, "Need an id to get");

						id = atoi(argv[3]);

						Database_get(conn, id);
						break;

				case 's':
						if (argc != 6)
								die(conn, "Need id, name, email to set");

						id = atoi(argv[3]);

						Database_set(conn, id, argv[4], argv[5]);
						Database_write(conn);
						break;

				case 'd':
						if (argc != 4)
								die(conn, "Need id to delete");

						id = atoi(argv[3]);

						Database_delete(conn, id);
						Database_write(conn);
						break;

				case 'l':
						Database_list(conn);
						break;
				default:
						die(conn, "Invalid action: c=create, g=get, s=set, d=del, l=list");
		}

		Database_close(conn);

		return 0;
}
