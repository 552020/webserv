// client.cpp
#include <iostream>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <arpa/inet.h>
#include <unistd.h>

void init_openssl()
{
	SSL_load_error_strings();
	OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl()
{
	EVP_cleanup();
}

int main()
{
	init_openssl();
	SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8443);
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

	connect(sock, (struct sockaddr *)&addr, sizeof(addr));

	SSL *ssl = SSL_new(ctx);
	SSL_set_fd(ssl, sock);

	if (SSL_connect(ssl) == 1)
	{
		const char *msg = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
		SSL_write(ssl, msg, strlen(msg));

		char buf[1024];
		int bytes = SSL_read(ssl, buf, sizeof(buf) - 1);
		buf[bytes] = '\0';
		std::cout << "Received: " << buf << std::endl;
	}
	else
	{
		ERR_print_errors_fp(stderr);
	}

	SSL_free(ssl);
	close(sock);
	SSL_CTX_free(ctx);
	cleanup_openssl();

	return 0;
}
