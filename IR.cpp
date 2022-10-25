/*
 * IR.cpp
 *
 * Created: 26/08/2022 08:48:12
 *  Author: Rodrigo
 */ 

#include "IR.h"

void LedON()
{
	DDRD |= (1<<3);
}

//================================================================
void LedOFF()
{
	DDRD &= ~(1<<3);
}
//================================================================
void IRtimerTX()
{
	 TCCR2A = (1<<WGM20)|(1<<COM2B1);
	 TCCR2B = (1<<WGM22) | (1<<CS20);
	 OCR2A = 209;
	 OCR2B = 61;
	 TCNT2 = 0;
	 LedOFF();
}
//================================================================
void delay(int t)
{
	unsigned char duracao = TCCR0B , tempo = t/64;
	TCCR0B |=(1<<CS02)|(1<<CS00);
	TCNT0 =0;
	while (TCNT0 <= tempo)
	{
		;
	}

	TCCR0B = duracao;
}
////================================================================

void mark(int tempo)
{
	LedON();
	delay(tempo);
	LedOFF();
	
}
//================================================================
void space(int tempo)
{
	delay(tempo);
}
//================================================================

void Transmissor_envia_rept() 
{
	mark(9000);
	space(2250);
	mark(560);		
}
//================================================================

void enviapulsos(unsigned long msg, unsigned char stop)
 {

	
	for (unsigned char bit = 0; bit < 32; bit++, msg >>= 1)
	{
		if(msg & 1)
		{

			mark(560);
			space(1690);
		}
		else
		{
			mark(560);
			space(560);
		}
	}
		
	
	if (stop ==1) 
	{
		mark(560);
	}
	
}


//================================================================


void envia(unsigned long msg, unsigned char qtdrepete) 
{
	
	mark(9000);
	space(4500);
	enviapulsos(msg, 1);
	
	for (unsigned char i = 0; i < qtdrepete; ++i) 
	{
		if (i == 0)
		 {
			_delay_ms(48);
		 } 
		 else 
			{
				_delay_ms(110);
			}
		Transmissor_envia_rept();
	}

}
//================================================================
void Transmissor_envia(unsigned char endereco,unsigned char  comando, unsigned char qtdrepete)
{
	unsigned long msg;
	unsigned long comandoIV, comand =comando;
	
	comandoIV = ~comando;
	msg = comandoIV;
	msg = (msg << 24)|(comand<<16)|endereco;
	
	envia(msg,qtdrepete);
}

//================================================================

//====================================================================FIM=TRANSMISSOR================================================================

//====================================================================SLAVE==========================================================================



volatile Tipo_IRparametros PARAMETROS;

void IRtimerRX()
{
	TCCR2A = (1 << WGM21);
	TCCR2B =(1 << CS21);
	TIMSK2 = (1 << OCIE2A);
	OCR2A = 100;
	TCNT2 = 0;
	
	sei();
}

void Receptor_habilitar()
{
	IRtimerRX();
	PARAMETROS.status = 0x02;
	PARAMETROS.qtd_duracao = 0x00;
	DDRD &= ~PINRX;
	PORTD |= PINRX;
}


//=====================================================================================================================================================

int tolerancia_baixo(int valor)
{
	return int(0.015 * (float)valor);
}


int tolerancia_alto(int valor)
{
	return int(1.025 * (float)valor);
}


int rajadapulsos(int pulsosmedida, int valor)
{

	return pulsosmedida >= tolerancia_baixo(valor + 100) && pulsosmedida <= tolerancia_alto(valor + 100);
}

int bitespaco(int pulsosmedida, int valor)
{

	return pulsosmedida >= tolerancia_baixo(valor - 100) && pulsosmedida <= tolerancia_alto(valor - 100);
}



unsigned long decodificar_bits(Tipo_Decodificado *dados)
{
	unsigned long dado = 0;
	int IDbit = 1; 
	unsigned char comando, comandoIV ;

	if (!rajadapulsos(dados->bit[IDbit], 9000))
	{
		return 0;
	}
	IDbit++;
	
	if ((PARAMETROS.qtd_duracao == 4) && (bitespaco(dados->bit[IDbit], 2250)) && (rajadapulsos(dados->bit[IDbit+1], 560)) )
	{
		dados->dado = 0xFFFFFFFF;
		PARAMETROS.qtd_duracao = 0;
		return 1;
	}
	
	if (PARAMETROS.qtd_duracao < 68 )
	{
		return 0;
	}
	
	
	if (!bitespaco(dados->bit[IDbit], 4500))
	{
		return 0;
	}
	IDbit++;
	
	for (int i = 0; i < 32; i++) 
	{
		if (!rajadapulsos(dados->bit[IDbit],560))
		{
			return 0;
		}
		IDbit++;
		if (bitespaco(dados->bit[IDbit], 1600)) 
		{
			dado = (dado << 1) | 1;
		}
		else 
			if (bitespaco(dados->bit[IDbit], 560))
			{
				dado <<= 1;
			}
			else
				{
					return 0;
				}
		IDbit++;
	}
	
	for (int i =0; i<32; i++)
	{
		dados->dado <<= 1;
		if(dado & 0x01)
		{
			dados->dado  |= 1;
			
		}
		dado =dado >> 1;
	}
     comandoIV = (dados->dado & 0xFF000000) >>24;
     comando = (dados->dado & 0x00FF0000) >>16;
     dados->comando = comando;
	 dados->endereco =(dados->dado & 0x000000FF);
   
    if (!((unsigned char)comandoIV == (unsigned char)~comando))
    {
		return 0;
    }
	
	
	PARAMETROS.qtd_duracao = 0;
	return 1;
}


void iniciarparametros()
{
	PARAMETROS.status = 0x02;
	PARAMETROS.qtd_duracao = 0;
}

unsigned char decodificar(Tipo_Decodificado *valor)
{
	valor->bit = PARAMETROS.dados;
	valor->qtd_duracao = PARAMETROS.qtd_duracao;
	if (PARAMETROS.status != 0x05)
	{
		return 0;
	}
	if (decodificar_bits(valor))
	{
		return 1;
	}
	
	iniciarparametros();
	return 0;
}


ISR(TIMER2_COMPA_vect)
{
	unsigned char irpin;
	if (PIND  & PINRX )
	{
		irpin =1;
	}
	else
		{
		  irpin =0;
		}
		
	PARAMETROS.duracao++;
	
	if (PARAMETROS.qtd_duracao >= 70)
	{
		PARAMETROS.status = 0x05;
	}
	
	switch(PARAMETROS.status)
	{
		case 0x02:
		if (irpin == 0)
		 {
			if (PARAMETROS.duracao < 100)
			{
			
				PARAMETROS.duracao = 0;
			}
			else
			{
				PARAMETROS.qtd_duracao = 0;
				PARAMETROS.dados[PARAMETROS.qtd_duracao++] = PARAMETROS.duracao;
				PARAMETROS.duracao = 0;
				PARAMETROS.status = 0x03;
			}
		}
		break;
		case 0x03: 
		if (irpin == 1)
		{   
			PARAMETROS.dados[PARAMETROS.qtd_duracao++] = PARAMETROS.duracao;
			PARAMETROS.duracao = 0;
			PARAMETROS.status = 0x04;
		}
		break;
		case 0x04: 
		if (irpin == 0)
		{ 
			PARAMETROS.dados[PARAMETROS.qtd_duracao++] = PARAMETROS.duracao;
			PARAMETROS.duracao = 0;
			PARAMETROS.status = 0x03;
		}
		else 
			{ 
				if (PARAMETROS.duracao > 100)
				{
				
					PARAMETROS.status = 0x05;
				}
			}
		break;
		case 0x05: 
		if (irpin == 0) 
		{ 
			PARAMETROS.duracao = 0;
		}
		break;
	}
}