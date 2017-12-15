$(document).ready(
	function(){
		$.get("obterListaSensores", function(listaJson){
			var numeroSensor = 1;
			var sensoresHtml = "";
			while(listaJson[numeroSensor]){
				sensoresHtml += "<option value='" + listaJson[numeroSensor] + "'>" + listaJson[numeroSensor] + "</option>"
				numeroSensor = numeroSensor + 1;
			}
				$("#listaSensores").html(sensoresHtml);
				$("#sensoresHistorico").html(sensoresHtml);
		});
	}
);

//variavel que vai disparar a leitura a cada t segundos definido pelo usuario
var intervalo;

//intervalo de tempo entre as medidas em tempo-real
var tempo;

//array que guarda os pontos das medidas em tempo real
var pontos_grafico = [];

//set que guarda os endereços encontrados na pesquisa e seus respectivos valores
var resultados = {}

//sets que guarda os arrays com os ids das requisicoes e das respostas recebidas pela leitura em tempo real
lista_ids_requisicao = {};
lista_ids_resposta = {};
lista_timestamps_sensor = {};
lista_diff_tempos = {};

var obterRecursos = function(){

	endereco_para_enviar = {'endereco' : $("#listaSensores").val()};

	$.post("obterRecursos/", JSON.stringify(endereco_para_enviar), function(data){
		$("#recurso").empty();
		var recursosHtml = '';
		data['recursos'].forEach(function(recurso){
			recursosHtml += "<option value='" + recurso + "'>" + recurso + "</option>";				
		});			
		$("#recurso").html(recursosHtml);
	});

};

var obterDadosSensor = function(){

	endereco_para_enviar = {'endereco' : $("#listaSensores").val()};

	$.post("obterDadosSensor/", JSON.stringify(endereco_para_enviar), function(data){
		$("#luminosidade").text(data['light']);
		$("#temperatura").text(data['temperature']);
	});
};

var iniciarLeituraContinuada = function(){

	tempo = parseFloat($("#tempo").val());

	enderecos = $("#listaSensores").val();
	intervalo = setInterval(function(){

		for(i = 0; i < enderecos.length; i++){

			dados_para_enviar = {'endereco' : enderecos[i], 'recurso' : $("#recurso").val(), 'idSensor' : i};

			$.ajax({
				type: "POST",
				url: "obterLeitura/",
				data: JSON.stringify(dados_para_enviar),
				success: function(data){
					var i = data['idS'];

					if (!pontos_grafico[i]){
						pontos_grafico[i] = [];
					}
					var now = new Date();
					pontos_grafico[i].push([now, data['d']]);

					if (!lista_ids_requisicao[enderecos[i]]){
						lista_ids_requisicao[enderecos[i]] = [];
					}						
					lista_ids_requisicao[enderecos[i]].push(data['iReq']);

					if (!lista_ids_resposta[enderecos[i]]){
						lista_ids_resposta[enderecos[i]] = [];
					}						
					lista_ids_resposta[enderecos[i]].push(data['iRes']);
					
					if (!lista_timestamps_sensor[enderecos[i]]){
						lista_timestamps_sensor[enderecos[i]] = [];
					}
					
					var timestamp;
					
					if(String(data['t']) == "-1"){
						timestamp = timestamp = new Date().toISOString()
					}
					else{
						timestamp = new Date(data['t']).toISOString();
					}
					
					lista_timestamps_sensor[enderecos[i]].push(timestamp); //change to miliseconds
					
					if (!lista_diff_tempos[enderecos[i]]){
						lista_diff_tempos[enderecos[i]] = [];
					}
					lista_diff_tempos[enderecos[i]].push(now.getTime() - new Date(timestamp).getTime());

					var dinamic_label;
					if ($('#recurso option:selected').text() == 'light'){
						dinamic_label = 'Luz (lux)';
					}
					if ($('#recurso option:selected').text() == 'temperature'){
						dinamic_label = 'Temperatura (ºC)';
					}
					$.jqplot('graficoTempoReal',  pontos_grafico, {
						legend: { 
							show: true, 
							labels: enderecos,
							location: 'sw', 
							placement: 'outside',
							marginLeft: 300
						},
						axes: {
							xaxis:{
								label:'Tempo',
								renderer:$.jqplot.DateAxisRenderer,
								labelRenderer:$.jqplot.CanvasAxisLabelRenderer,
								tickRenderer:$.jqplot.CanvasAxisTickRenderer,
								tickOptions:{formatString:'%T', angle:15}
							},
							yaxis:{
								label:dinamic_label,
								labelRenderer: $.jqplot.CanvasAxisLabelRenderer
							}
						}
					}).replot();
				},
				error: function(jqXHR, textStatus, errorThrown) {
					alert(textStatus + " " + errorThrown);
				},
				async:true
			});
		}
	}, tempo*1000);
	alert("Leitura iniciada");


	$("#tempo").attr('disabled','disabled');
	$("#recurso").attr('disabled','disabled');
	$("#listaSensores").attr('disabled','disabled');
	$("#start").hide();
	$("#stop").show();

};

var interromperLeituraContinuada = function(){
	clearInterval(intervalo);

	alert("Leitura interrompida");
	$("#tempo").removeAttr('disabled');
	$("#recurso").removeAttr('disabled');
	$("#listaSensores").removeAttr('disabled');
	$("#stop").hide();
	$("#start").show();
};

var limparGrafico = function(nome){
	$('#'+nome).empty();
	pontos_grafico = [];
	lista_ids_requisicao = {};
	lista_ids_resposta = {};
};

var salvarDadosObtidos = function(){

	var experimento;
	
	$.get("obterNomeExperimento/", function(data){
		experimento = prompt("Digite o nome do experimento a ser salvo", data['nome']);
		if(!experimento || experimento == ""){
			alert("Operação cancelada pelo usuário");
		}
		else{
			objeto_para_enviar = {'enderecos' : $("#listaSensores").val(),
						'recurso' : $("#recurso").val(),
						'valores' : pontos_grafico,
						'idRequisicao' : lista_ids_requisicao,
						'idResposta' : lista_ids_resposta,
						'timestamps' : lista_timestamps_sensor,
						'diffTempos' : lista_diff_tempos,
						'experimento' : experimento};

			$.post("salvarDadosObtidos/", JSON.stringify(objeto_para_enviar), function(data){
				if(data['experimento']){
					alert(data['mensagem'] + " : " + data['experimento']);
				}
				
				else{
					alert(data['mensagem']);
				}

			});
		}
	});


};

var obterHistorico = function(){
	objeto_para_enviar = {'dataInicial' : $("#de").val(),
				'dataFinal' : $("#ate").val(),
				'recurso' : $("#recursoHistorico").val(),};

	$("#listaExperimentosEncontrados").empty();

	$.post("obterHistorico/", JSON.stringify(objeto_para_enviar), function(data){
		limparGrafico('pesquisaHistorico');
		resultados = {}

		data['listaResultados'].forEach(function(resultado){
/*
 
O set "resultados" possui como chaves os nomes dos experimentos e como valores um set com o endereço como chave
e um array com duplas (data, medida) como valor.
Este bloco if é responsável por criar a chave caso ela ainda não exista

*/
			if(!resultados[resultado['experimento']]){
				resultados[resultado['experimento']] = {};
			}
/*
Eu preciso agora criar chaves para os enderecos, pois meu banco possui varios enderecos gravados em um unico experimento
*/
			if(!resultados[resultado['experimento']][resultado['endereco']]){
				resultados[resultado['experimento']][resultado['endereco']] = [];
			}

			resultados[resultado['experimento']][resultado['endereco']].push([new Date(resultado['dataInclusao']), resultado['valor']]);
		});

		var listaExperimentosEncontradosHtml = '<option value=""> - </option>';

		for(var experimento in resultados){
			listaExperimentosEncontradosHtml += "<option value='"+experimento+"'>"+experimento+"</option>"
		}
		$("#listaExperimentosEncontrados").html(listaExperimentosEncontradosHtml);
	});
};

var exibirGraficoHistorico = function(){	

	var pontos = [];
	var experimento;
	var enderecos = [];

	if($("#listaExperimentosEncontrados").val() != ""){
		experimento = resultados[$("#listaExperimentosEncontrados").val()];
	}

	for(var endereco in experimento){
		enderecos.push(endereco);
		pontos.push(experimento[endereco]);
	}
	var dinamic_label;
	if ($('#recursoHistorico option:selected').text() == 'light'){
		dinamic_label = 'Luz (lux)';
	}
	if ($('#recursoHistorico option:selected').text() == 'temperature'){
		dinamic_label = 'Temperatura (ºC)';
	}

	$.jqplot('pesquisaHistorico', pontos, {
		legend: { 
		    show: true, 
		    labels: enderecos,
		    location: 'sw', 
		    placement: 'outside',
		    marginLeft: 300
		},
		axes: {
			xaxis:{
			  renderer:$.jqplot.DateAxisRenderer,
			  labelRenderer:$.jqplot.CanvasAxisLabelRenderer,
			  tickRenderer:$.jqplot.CanvasAxisTickRenderer,
			  tickOptions:{formatString:'%#c', angle:15},
			  min:pontos[0][0][0],
			  max:pontos[pontos.length - 1][0][0]
			},
			yaxis:{
				label:dinamic_label,
				labelRenderer: $.jqplot.CanvasAxisLabelRenderer
			}
		}
	}).replot({resetAxes:true});

};
