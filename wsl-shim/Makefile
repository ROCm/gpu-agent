AMDSMI_INC := ../sw/nic/third-party/rocm/wsl_amd_smi_lib/include
CXX        := g++
CXXFLAGS   := -std=c++17 -shared -fPIC -O2 -Wall \
               -I$(AMDSMI_INC)
LDFLAGS    := -ldl

TARGET     := libamd_smi_wsl_shim.so
SRC        := wsl_amdsmi_shim.cc

INSTALL_DIR := /usr/local/metrics/lib

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

install: $(TARGET)
	install -m 0755 $(TARGET) $(INSTALL_DIR)/$(TARGET)

clean:
	rm -f $(TARGET)
