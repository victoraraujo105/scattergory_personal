# USO:
# <$ make> para compilar
# <$ make clean> para limpar arquivos criados


TARGET = scattergory	# executáveis

CC = gcc	# compilador

# diretórios

SDIR = src	# arquivos fonte
ODIR = obj	# arquivos objeto
IDIR = include	# header (cabeçalhos)

# flags

CFLAGS = -Wall -ansi -pedantic -I$(IDIR)
LDFLAGS =	# flags requeridas por certas bibliotecas, como <-lm> por <math.h>

# nomes de arquivos

_SRC = main.c	# arquivos fonte <*.c>
SRC = $(_SRC:%=$(SDIR)/%)	# prefixando diretorio ao nome dos arquivos fonte <*.c>

_OBJ = $(_SRC:%.c=%.o)	# arquivos objeto, trocando extensão dos arquivos fonte para <.o>
OBJ = $(_OBJ:%=$(ODIR)/%)	# prefixando diretorio ao nome dos arquivos objeto <*.o>

_INCLUDE =	# arquivos header <*.h>
INCLUDE = $(_INCLUDE:%=$(IDIR)/%)



.PHONY: all clean	# nome dos targets que não são arquivos,
	# ignora a possível existência de arquivos com mesmo nome na raiz do projeto


all: $(TARGET)	# regra principal, garante a existência dos executáveis

$(TARGET): $(OBJ)	# regra que liga arquivos objeto
	@echo "\n\nLigando arquivos objeto $(OBJ:%=<%>)..."
	@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "\nCompilado! digite <./$@> para executar."

$(ODIR)/%.o: $(SDIR)/%.c $(INCLUDE) $(ODIR)	# regra que compila arquivos objeto
	@echo "\n\nGerando <$@>..."
	@$(CC) -c -o $@ $< $(CFLAGS)
	@echo "\nArquivo <$@> gerado!"

$(ODIR):	# regra que cria diretório dos arquivos objeto, caso não exista
	@echo "Criando diretório <./obj>..."
	@mkdir -p $@
	@echo "\nDiretório <.obj> criado!"

clean:	# regra que apaga arquivos gerados
	@echo "Deletando arquivos gerados..."
	@rm -rf $(ODIR) $(TARGET) *~ $(SDIR)/*~ $(IDIR)/*~
	@echo "\nArquivos gerados deletados!"
