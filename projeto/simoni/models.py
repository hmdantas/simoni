from __future__ import unicode_literals

from django.db import models

class Medida(models.Model):
	endereco = models.CharField(max_length=200)
	dataInclusao = models.DateTimeField('date published')
	timestampSensor = models.DateTimeField('date sensor')
	diffTempos = models.IntegerField('diff in milisseconds')
	recurso = models.CharField(max_length=200)
	valor = models.FloatField(default=0.0)
	experimento = models.CharField(max_length=200)
	idRequisicao = models.IntegerField(default=0)
	idResposta = models.IntegerField(default=0)

