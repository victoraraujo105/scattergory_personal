# USO:
# <$ make> para compilar
# <$ make clean> para limpar arquivos criados


TARGET = scattergory	# executáveis

CC = gcc	# compilador

# diretórios

# arquivos fonte
SDIR = src
# arquivos objeto
ODIR = obj
# header (cabeçalhos)
IDIR = include

# flags

CFLAGS = -Wall -std=gnu99 -pedantic -I$(IDIR)
LDFLAGS =	# flags requeridas por certas bibliotecas, como <-lm> por <math.h>

# nomes de arquivos

_SRC = main.c	# arquivos fonte <*.c>
SRC = $(_SRC:%=$(SDIR)/%)	# prefixando diretorio ao nome dos arquivos fonte <*.c>

_OBJ = $(_SRC:%.c=%.o)	# arquivos objeto, trocando extensão dos arquivos fonte para <.o>
OBJ = $(_OBJ:%=$(ODIR)/%)	# prefixando diretorio ao nome dos arquivos objeto <*.o>

_INCLUDE = main.h # arquivos header <*.h>
INCLUDE = $(_INCLUDE:%=$(IDIR)/%)



.PHONY: all clean	# nome dos targets que não são arquivos,
	# ignora a possível existência de arquivos com mesmo nome na raiz do projeto


all: $(TARGET)	# regra principal, garante a existência dos executáveis

$(TARGET): $(OBJ)	# regra que liga arquivos objeto
	@echo "Ligando arquivos objeto $(OBJ:%=<%>)...\n"
	@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Compilado! digite <./$@> para executar."

$(ODIR)/%.o: $(SDIR)/%.c $(INCLUDE) $(ODIR)	# regra que compila arquivos objeto
	@echo "Gerando <$@>..."
	@$(CC) -c -o $@ $< $(CFLAGS)
	@echo "\nArquivo <$@> gerado!\n\n"

$(ODIR):	# regra que cria diretório dos arquivos objeto, caso não exista
	@echo "Criando diretório <./obj>..."
	@mkdir -p $@
	@echo "\nDiretório <.obj> criado!\n\n"

clean:	# regra que apaga arquivos gerados
	@echo "Deletando arquivos gerados..."
	@rm -rf $(ODIR) $(TARGET) *~
	@echo "\nArquivos gerados deletados!"
