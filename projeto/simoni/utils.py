from coapthon.client.helperclient import HelperClient
import json
import re

def obterEnderecoRequisicaoPost(body):

	dadosJson = json.loads(body)

	#necessario para tratar o unicode
	endereco_sensor = str(dadosJson['endereco'])

	return endereco_sensor

def obterClienteCoap(host, port):
	client = HelperClient(server=(host, port))
	return client

def getPayloadCoapPorHostPortaEPath(host, port, path):
	client = obterClienteCoap(host, port)
	response = client.get(path)
	client.stop()
	return response.payload

def discoverPayload(host, port):
	client = obterClienteCoap(host, port)
	return client.discover().payload

def converteDiscoverPayloadDictRecursos(payload):
	lista_recursos = re.findall("\<\/(.*?)>", payload)
	lista_recursos.remove('.well-known/core')

	recursos = { "recursos" : []}
	for recurso in lista_recursos:
		recursos['recursos'].append(recurso)
	return recursos