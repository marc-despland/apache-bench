FROM ubuntu
MAINTAINER Marc Despland <marc.despland@orange.com>
RUN apt-get update && apt-get install -y apache2 vim net-tools
RUN ln -s /etc/apache2/mods-available/proxy.load /etc/apache2/mods-enabled/proxy.load
RUN ln -s /etc/apache2/mods-available/proxy_http.load /etc/apache2/mods-enabled/proxy_http.load
RUN ln -s /etc/apache2/mods-available/rewrite.load /etc/apache2/mods-enabled/rewrite.load
RUN mkdir -p /var/run/apache2; mkdir -p /var/lock/apache2
RUN update-rc.d apache2 disable
COPY 000-default.conf /etc/apache2/sites-available/
COPY choose_mpm.sh /bin/
RUN chmod a+rx /bin/choose_mpm.sh
RUN rm /etc/apache2/mods-enabled/mpm*
RUN /bin/choose_mpm.sh prefork
COPY start_apache.sh /bin/
RUN chmod a+rx /bin/start_apache.sh
#COPY limits.conf /etc/security/
WORKDIR /etc/apache2
EXPOSE 80
CMD /bin/bash
