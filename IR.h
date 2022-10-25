/*
 * IR.h
 *
 * Created: 26/08/2022 08:47:54
 *  Author: Rodrigo
 */ 


#ifndef IR_H_
#define IR_H_
#include <avr/io.h>
#include <stdint.h>

#ifndef  F_CPU
#define  F_CPU 16000000UL
#endif


#include <util/delay.h>
#include <avr/interrupt.h>


#define  PINRX (1<<2)





typedef struct
{
	unsigned char status;
	unsigned int duracao;
	unsigned int dados[70];
	unsigned char qtd_duracao;
}Tipo_IRparametros;

typedef struct
{
	unsigned long dado;
	unsigned char endereco;
	unsigned char comando;
	volatile unsigned int *bit;
	int qtd_duracao;
}Tipo_Decodificado;


void IRtimerTX();
void Transmissor_envia(unsigned char endereco,unsigned char  comando, unsigned char qtdrepete); 

void Receptor_habilitar();
unsigned char decodificar(Tipo_Decodificado *results);




#endif /* IR_H_ */