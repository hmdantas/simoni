# -*- coding: utf-8 -*-
# Generated by Django 1.10.6 on 2017-06-30 15:36
from __future__ import unicode_literals

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('simoni', '0003_medida_difftempos'),
    ]

    operations = [
        migrations.AlterField(
            model_name='medida',
            name='diffTempos',
            field=models.IntegerField(verbose_name='diff in milisseconds'),
        ),
    ]
