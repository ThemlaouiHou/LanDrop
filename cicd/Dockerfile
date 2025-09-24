############################################
##              Base install              ##
############################################
FROM debian

RUN apt update
RUN apt install -y software-properties-common
RUN apt update -y && apt upgrade -y

############################################
##                 Ninja                  ##
############################################
# Install Ninja
RUN apt install -y ninja-build


############################################
##                Doxygen                 ##
############################################
# Install Doxygen
RUN apt install -y doxygen


############################################
##                  Qt                    ##
############################################
# Install Qt dependencies
RUN apt-get install -y \
  ca-certificates \
  openssl \
  build-essential \
  cmake \
  libgl-dev \
  libvulkan-dev \
  libssl-dev \
  libgl1-mesa-dev \
  python3 \
  python3-pip \
  libqt5x11extras5  

# Install Qt 6.8.3
RUN pip3 install --break-system-packages aqtinstall
RUN mkdir opt/Qt
WORKDIR /opt/Qt
RUN aqt install-qt linux desktop 6.8.3


############################################
##              Default shell             ##
############################################
CMD ["bash"]
