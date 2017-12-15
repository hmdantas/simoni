from django import forms

class MedidaForm(forms.Form):
	endereco = forms.CharField(label='Endereco', widget=forms.HiddenInput())
	dataInclusao = forms.DateTimeField('date published', widget=forms.HiddenInput())
	grandeza = forms.CharField(max_length=200, widget=forms.HiddenInput())
	valor = forms.IntegerField()
