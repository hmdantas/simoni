# Create your views here.
from django.http import HttpResponse
from django.shortcuts import render
from django.utils.timezone import get_current_timezone
from coapthon.client.helperclient import HelperClient
import urllib2
import json
import datetime
import time

from django.utils.encoding import python_2_unicode_compatible

from simoni.models import Medida

from django.utils import timezone

from utils import *

idMsg = 0
porta_padrao = 5683

def index(request):
	return render(request, 'simoni/index.html')
	
def obterNomeExperimento(request):
	idExperimento = 0
	
	#obtem o nome do experimento mais recente
	lista_experimentos = Medida.objects.filter(dataInclusao__lte=(timezone.now()))
	
	if(len(lista_experimentos) > 0):
		nome_experimento_mais_recente = lista_experimentos.order_by('-dataInclusao')[0].experimento
		lista_nome = nome_experimento_mais_recente.split()
		if(str(lista_nome[2]) == timezone.now().strftime('%d-%m-%Y')):
			idExperimento = int(lista_nome[1])
		
	idExperimento = idExperimento + 1	
	jsonResposta = {}
	jsonResposta['nome'] = "experimento " + str(idExperimento) + " " + timezone.now().strftime('%d-%m-%Y')
	jsonResposta = json.dumps(jsonResposta)

	return HttpResponse(jsonResposta, content_type="application/json")
	

def obterListaSensores(request):

	#dado_do_sensor = urllib2.urlopen("http://[aaaa::212:7401:1:101]/").read()
	dado_do_sensor = urllib2.urlopen("http://[2001:660:5307:3100::b080]/").read()

	#tratando as aspas para conversao apropriada para json
	dado_do_sensor = dado_do_sensor.replace("'", "\"")
	return HttpResponse(dado_do_sensor, content_type="application/json")

def obterLeitura(request):
	#import pdb; pdb.set_trace()

	if(request.method == "POST"):
		dadosJson = json.loads(request.body)

		#necessario para tratar o unicode
		endereco_sensor = str(dadosJson['endereco'])
		recurso = str(dadosJson['recurso'])
		idSensor = str(dadosJson['idSensor'])

		global idMsg

		idMsg = (idMsg + 1) % 10000
		#TODO! a passagem do id por query string tem que ser usando as estruturas do Coapthon
		dado_do_sensor = getPayloadCoapPorHostPortaEPath(endereco_sensor, porta_padrao, recurso+"?idSensor="+idSensor+"&idMsg="+str(idMsg))

		dado_do_sensor = dado_do_sensor.replace("'", "\"")
		return HttpResponse(dado_do_sensor, content_type="application/json")

def salvarDadosObtidos(request):

	if(request.method == "POST"):

		dadosJson = json.loads(request.body)
		experimento_cadastrado = Medida.objects.filter(experimento = str(dadosJson['experimento']))
		#import pdb; pdb.set_trace() #testar o valores
		if(len(experimento_cadastrado)):

			erroJson = {}
			erroJson['mensagem'] = "Ja existe um experimento com esse nome"
			erroJson['experimento'] = str(dadosJson['experimento'])

			erroJson = json.dumps(erroJson)

			return HttpResponse(erroJson, content_type="application/json")

		else:
			valores = dadosJson['valores']
			idRequisicao = dadosJson['idRequisicao']
			idResposta = dadosJson['idResposta']
			timestamps = dadosJson['timestamps']
			diffTempos = dadosJson['diffTempos']

			#necessario pensar num modo de garantir a ordenacao!!!
			enderecos = dadosJson['enderecos']
			sucessoJson = {}

			if(len(enderecos) != len(valores)):
				sucessoJson['mensagem'] = "Nao foi possivel salvar os dados."
			else:		

				for i in range(len(enderecos)):
					for v in range(len(valores[i])):
						dado = Medida(endereco = str(enderecos[i]), 
						dataInclusao = isodate_to_tz_datetime(valores[i][v][0]), 
						recurso = str(dadosJson['recurso']), 
						valor = valores[i][v][1], 
						experimento = str(dadosJson['experimento']), 
						idRequisicao = idRequisicao[enderecos[i]][v], 
						idResposta = idResposta[enderecos[i]][v],
						timestampSensor = isodate_to_tz_datetime(timestamps[enderecos[i]][v]),
						diffTempos = diffTempos[enderecos[i]][v])
						dado.save()
				sucessoJson['mensagem'] = "sucesso!"

			sucessoJson = json.dumps(sucessoJson)

			global idMsg

			idMsg = 0

			return HttpResponse(sucessoJson, content_type="application/json")
			
def isodate_to_tz_datetime(isodate):
    """
    Convert an ISO date string 2011-01-01 into a timezone aware datetime that
    has the current timezone.
    """
    date = datetime.datetime.strptime(isodate, '%Y-%m-%dT%H:%M:%S.%fZ')
    current_timezone = timezone.get_current_timezone()
    return current_timezone.localize(date, is_dst=None)

def obterRecursos(request):
    #import pdb; pdb.set_trace()

	if(request.method == "POST"):

		dadosJson = json.loads(request.body)

		enderecos_sensor = dadosJson['endereco']

		dado_do_sensor = {}

		for endereco in enderecos_sensor:
			if('recursos' not in dado_do_sensor):
				dado_do_sensor['recursos'] = []

			payload = discoverPayload(endereco, porta_padrao)
			
			dado = converteDiscoverPayloadDictRecursos(payload)

			if(len(dado_do_sensor['recursos']) == 0):
				dado_do_sensor['recursos'] = dado['recursos']
			else:
				dado_do_sensor['recursos'] = [i for i in dado_do_sensor['recursos'] if i in dado['recursos']]

		dado_do_sensor = json.dumps(dado_do_sensor)
		return HttpResponse(dado_do_sensor, content_type="application/json")

def obterHistorico(request):
	if(request.method == "POST"):

		dadosJson = json.loads(request.body)

		dataInicial = dadosJson['dataInicial']
		dataFinal = dadosJson['dataFinal']

		formato = "%d/%m/%Y" #este eh o formato que o datepicker estah trazendo

		#os dados foram recebidos como string. E necessario converte-los para date para executar a busca no banco

		#inicializando a variavel resultados
		resultados = ''

		if('recurso' in dadosJson):
			_recurso = str(dadosJson['recurso'])
			resultados = Medida.objects.filter(recurso = _recurso)

		if(dataInicial != '' and dataFinal == ''):
			dataInicial = datetime.datetime.strptime(dataInicial, formato)
			resultados = resultados.filter(dataInclusao__gte=dataInicial)

		elif(dataInicial == '' and dataFinal != ''):
			dataFinal = datetime.datetime.strptime(dataFinal, formato) + datetime.timedelta(days=1)
			resultados = resultados.filter(dataInclusao__lte=(dataFinal))

		elif(dataInicial != '' and dataFinal != ''):
			dataInicial = datetime.datetime.strptime(dataInicial, formato)
			dataFinal = datetime.datetime.strptime(dataFinal, formato) + datetime.timedelta(days=1)
			resultados = resultados.filter(dataInclusao__range=(dataInicial, dataFinal))

		resultadosJson = {}
		listaResultados = []
		for resultado in resultados:
			resultadoJson = {}#
			resultadoJson['endereco'] = resultado.endereco
			resultadoJson['dataInclusao'] = resultado.dataInclusao.strftime('%m/%d/%Y %H:%M:%S')
			resultadoJson['recurso'] = resultado.recurso
			resultadoJson['valor'] = resultado.valor
			resultadoJson['experimento'] = resultado.experimento
			listaResultados.append(resultadoJson)

		resultadosJson['listaResultados'] = listaResultados            
		resultadosJson = json.dumps(resultadosJson)
		return HttpResponse(resultadosJson, content_type="application/json")
