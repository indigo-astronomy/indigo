FROM ubuntu:bionic

RUN apt-get update && \
    apt-get install -y \
      sudo udev software-properties-common \
      build-essential autoconf autotools-dev libtool cmake libudev-dev libavahi-compat-libdnssd-dev libusb-1.0-0-dev fxload libcurl4-gnutls-dev libgphoto2-dev libz-dev git curl bsdmainutils && \
    sudo apt-add-repository -y ppa:mutlaqja/ppa && \
    sudo su -c "echo 'deb [trusted=yes] https://indigo-astronomy.github.io/indigo_ppa/ppa indigo main' > /etc/apt/sources.list.d/indigo.list" && \
    sudo apt-get update && \
    sudo apt-get upgrade -y && \
    sudo apt-get install -y libqhy libsbig indi-dsi && \
    sudo apt-get -y --purge autoremove && \
    rm -rf /var/lib/apt/lists/*


WORKDIR /app
COPY indigo-build.sh /app/indigo-build.sh
COPY bootstrap.sh /app/bootstrap.sh
RUN "/app/bootstrap.sh"

# cleanup
RUN rm ./indigo-build.sh ./bootstrap.sh

COPY run.sh /app/run.sh
ENTRYPOINT ["/app/run.sh"]
