from django.conf.urls import include,url
from simoni.views import *

#from django.contrib import admin
#admin.autodiscover()

urlpatterns = [
    # Examples:
    # url(r'^$', 'HelloWorld.views.home', name='home'),
    # url(r'^blog/', include('blog.urls')),

    #url(r'^admin/', include(admin.site.urls)),
    url(r'^$', include('simoni.urls')),
    url(r'^obterListaSensores/$', obterListaSensores),
    url(r'^obterLeitura/$', obterLeitura),
    url(r'^salvarDadosObtidos/$', salvarDadosObtidos),
    url(r'^obterRecursos/$', obterRecursos),
    url(r'^obterHistorico/$', obterHistorico),
	url(r'^obterNomeExperimento/$', obterNomeExperimento),
]
