FROM registry.gitlab.com/qcomms/cqptoolkit/runtime

RUN apt update
ADD chromium-*.deb /root/
RUN dpkg -i /root/*.deb ; apt --fix-broken install -qy
RUN chmod 777 /home

WORKDIR /home
ENV HOME /home
CMD ["/usr/bin/chromium-browser", "--no-sandbox"]
