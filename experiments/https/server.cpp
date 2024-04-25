#include <iostream>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include <netinet/in.h>
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

SSL_CTX *create_context()
{
	const SSL_METHOD *method;
	SSL_CTX *ctx;

	method = TLS_server_method();
	ctx = SSL_CTX_new(method);
	if (!ctx)
	{
		perror("Unable to create SSL context");
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}

	return ctx;
}

void configure_context(SSL_CTX *ctx)
{
	SSL_CTX_set_ecdh_auto(ctx, 1);

	// Load certificate and private key files.
	if (SSL_CTX_use_certificate_file(ctx, "ssl/cert.pem", SSL_FILETYPE_PEM) <= 0)
	{
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}

	if (SSL_CTX_use_PrivateKey_file(ctx, "ssl/key.pem", SSL_FILETYPE_PEM) <= 0)
	{
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}
}

int main()
{
	init_openssl();
	SSL_CTX *ctx = create_context();
	configure_context(ctx);

	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8443);
	addr.sin_addr.s_addr = INADDR_ANY;

	bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
	listen(server_fd, 1);
	std::cout << "Listening on port 443." << std::endl;

	while (1)
	{
		std::cout << "Waiting for connection..." << std::endl;
		struct sockaddr_in addr;
		uint len = sizeof(addr);
		SSL *ssl;
		int client = accept(server_fd, (struct sockaddr *)&addr, &len);

		ssl = SSL_new(ctx);
		SSL_set_fd(ssl, client);

		if (SSL_accept(ssl) <= 0)
		{
			ERR_print_errors_fp(stderr);
		}
		else
		{
			char buffer[1024] = {0};
			int bytes = SSL_read(ssl, buffer, sizeof(buffer) - 1); // Read client request
			if (bytes > 0)
			{
				std::cout << "Received request:" << std::endl;
				std::cout << buffer << std::endl;
				SSL_write(ssl, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nHello, HTTPS world!\r\n", 74);
			}
			else
			{
				ERR_print_errors_fp(stderr);
			}
		}

		SSL_shutdown(ssl);
		SSL_free(ssl);
		close(client);
	}

	close(server_fd);
	SSL_CTX_free(ctx);
	cleanup_openssl();
}
