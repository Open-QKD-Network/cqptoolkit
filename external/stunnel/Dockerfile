FROM registry.gitlab.com/qcomms/cqptoolkit/runtime

RUN apt update
ADD stunnel*.deb /root/
RUN dpkg -i /root/*.deb ; apt --fix-broken install -qy

ENTRYPOINT ["/usr/bin/stunnel"]

ADD stunnel.conf /etc/stunnel/stunnel.conf
