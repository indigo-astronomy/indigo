ifeq ($(OS_DETECTED),Darwin)
LDFLAGS += -lstdc++
else
LDFLAGS += -lstdc++ -ludev -ldl
endif
