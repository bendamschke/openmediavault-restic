# @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
# @author    OpenMediaVault Plugin Developers <plugins@omv-extras.org>
# @copyright Copyright (c) 2019-2023 OpenMediaVault Plugin Developers
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

{% set config = salt['omv_conf.get']('conf.service.restic') %}
{% set envVarDir = '/etc/restic' %}
{% set envVarPrefix = 'restic-envvar-' %}
{% set logFile = '/var/log/restic.log' %}
{% set scriptsDir = '/var/lib/openmediavault/restic' %}
{% set scriptPrefix = 'restic-' %}

configure_restic_envvar_dir:
  file.directory:
    - name: "{{ envVarDir }}"
    - user: root
    - group: root
    - mode: 700

remove_envvar_files:
  module.run:
    - file.find:
        - path: "{{ envVarDir }}"
        - iname: "{{ envVarPrefix }}*"
        - delete: "f"

{% for repo in config.repos.repo %}
{% set envVarFile = envVarDir ~ '/' ~ envVarPrefix ~ repo.uuid %}

configure_restic_envvar_{{ repo.uuid }}:
  file.managed:
    - name: "{{ envVarFile }}"
    - source:
      - salt://{{ tpldir }}/files/etc-restic_envvar.j2
    - context:
        config: {{ config | json }}
        repouuid: {{ repo.uuid }}
    - template: jinja
    - user: root
    - group: root
    - mode: 600
{% endfor %}

{% set envVarFile = envVarDir ~ '/' ~ envVarPrefix ~ 'creation' %}
configure_restic_envvar_creation:
  file.managed:
    - name: "{{ envVarFile }}"
    - source:
      - salt://{{ tpldir }}/files/etc-restic_envvar.j2
    - context:
        config: {{ config | json }}
        repouuid: "creation"
    - template: jinja
    - user: root
    - group: root
    - mode: 600
