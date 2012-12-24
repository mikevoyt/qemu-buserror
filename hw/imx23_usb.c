/*
 * imx23_usb.c
 *
 * Copyright: Michel Pollet <buserror@gmail.com>
 *
 * QEMU Licence
 */

/*
 * Implements the USB block of the imx23. This is just a case of instantiating
 * a ehci block, and have a few read only registers fr imx23 specifics
 */
#include "sysbus.h"
#include "imx23.h"
#include "hw/usb/hcd-ehci.h"
#include "hw/sysbus.h"
#include "qdev.h"

#define D(w)

enum {
    USB_MAX = 256 / 4,
};

typedef struct imx23_usb_state {
    SysBusDevice busdev;
    MemoryRegion iomem;

    uint32_t r[USB_MAX];
    qemu_irq irq_dma, irq_error;

    EHCIState ehci;
} imx23_usb_state;

static uint64_t imx23_usb_read(
        void *opaque, hwaddr offset, unsigned size)
{
    imx23_usb_state *s = (imx23_usb_state *) opaque;
    uint32_t res = 0;

    D(printf("%s %04x (%d) = ", __func__, (int)offset, size);)
    switch (offset >> 2) {
        case 0 ... USB_MAX:
            res = s->r[offset >> 2];
            break;
        default:
            qemu_log_mask(LOG_GUEST_ERROR,
                    "%s: bad offset 0x%x\n", __func__, (int) offset);
            break;
    }
    D(printf("%08x\n", res);)

    return res;
}

static void imx23_usb_write(void *opaque, hwaddr offset,
        uint64_t value, unsigned size)
{
    imx23_usb_state *s = (imx23_usb_state *) opaque;

    D(printf("%s %04x %08x(%d)\n", __func__, (int)offset, (int)value, size);)
    switch (offset) {
        case 0 ... USB_MAX:
            s->r[offset] = value;
            break;
        default:
            qemu_log_mask(LOG_GUEST_ERROR,
                    "%s: bad offset 0x%x\n", __func__, (int) offset);
            break;
    }
}

static const MemoryRegionOps imx23_usb_ops = {
    .read = imx23_usb_read,
    .write = imx23_usb_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static int imx23_usb_init(SysBusDevice *dev)
{
    imx23_usb_state *s = FROM_SYSBUS(imx23_usb_state, dev);
    EHCIState *u = &s->ehci;

    memory_region_init_io(&s->iomem, &imx23_usb_ops, s,
            "imx23_usb", 0x100);

    s->r[0] = 0xe241fa05;
    s->r[0x04 >> 2] = 0x00000015;
    s->r[0x08 >> 2] = 0x10020001;
    s->r[0x0c >> 2] = 0x0000000b;
    s->r[0x10 >> 2] = 0x40060910;
    s->r[0x14 >> 2] = 0x00000710;

    u->capsbase = 0x100;
    u->opregbase = 0x140;
    u->dma = &dma_context_memory;

    usb_ehci_initfn(u, DEVICE(dev));
    sysbus_init_irq(dev, &u->irq);

    memory_region_add_subregion(&u->mem, 0x0, &s->iomem);
    sysbus_init_mmio(dev, &u->mem);

    D(printf("%s created bus %s\n", __func__, u->bus.qbus.name);)
    /*
     * This is suposed to make companion ports that will support
     * slower speed devices. It's inspired from ehci/pci however
     * it doesn't work, right now...
     */
    int i;
    for (i = 0; i < NB_PORTS; i += 2) {
        DeviceState * d = qdev_create(NULL, "sysbus-ohci");
        qdev_prop_set_string(d, "masterbus", u->bus.qbus.name);
        qdev_prop_set_uint32(d, "firstport", i);
        qdev_prop_set_uint32(d, "num-ports", 2);
        qdev_init_nofail(d);
        sysbus_connect_irq(SYS_BUS_DEVICE(d), 0, u->irq);
    }

    return 0;
}

static void imx23_usb_class_init(ObjectClass *klass, void *data)
{
    SysBusDeviceClass *sdc = SYS_BUS_DEVICE_CLASS(klass);

    sdc->init = imx23_usb_init;
}

static TypeInfo imx23_usb_info = {
    .name          = "imx23_usb",
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(imx23_usb_state),
    .class_init    = imx23_usb_class_init,
};

static void imx23_usb_register(void)
{
    type_register_static(&imx23_usb_info);
}

type_init(imx23_usb_register)

#undef D
#define D(w)

enum {
    USBPHY_PWD = 0x0,
    USBPHY_TX = 0x1,
    USBPHY_RX = 0x2,
    USBPHY_CTRL = 0x3,
    USBPHY_MAX = 10,
};
typedef struct imx23_usbphy_state {
    SysBusDevice busdev;
    MemoryRegion iomem;

    uint32_t r[USBPHY_MAX];
} imx23_usbphy_state;

static uint64_t imx23_usbphy_read(void *opaque, hwaddr offset,
        unsigned size)
{
    imx23_usbphy_state *s = (imx23_usbphy_state *) opaque;
    uint32_t res = 0;

    D(printf("%s %04x (%d) = ", __func__, (int)offset, size);)
    switch (offset >> 4) {
        case 0 ... USBPHY_MAX:
            res = s->r[offset >> 4];
            break;
        default:
            qemu_log_mask(LOG_GUEST_ERROR,
                    "%s: bad offset 0x%x\n", __func__, (int) offset);
            break;
    }
    D(printf("%08x\n", res);)

    return res;
}

static void imx23_usbphy_write(void *opaque, hwaddr offset,
        uint64_t value, unsigned size)
{
    imx23_usbphy_state *s = (imx23_usbphy_state *) opaque;
    uint32_t oldvalue = 0;

    D(printf("%s %04x %08x(%d) = ", __func__, (int)offset, (int)value, size);)
    switch (offset >> 4) {
        case 0 ... USBPHY_MAX:
            oldvalue = imx23_write(&s->r[offset >> 4], offset, value, size);
            break;
        default:
            qemu_log_mask(LOG_GUEST_ERROR,
                    "%s: bad offset 0x%x\n", __func__, (int) offset);
            return;
    }
    switch (offset >> 4) {
        case USBPHY_CTRL:
            if ((oldvalue ^ s->r[USBPHY_CTRL]) == 0x80000000
                    && !(oldvalue & 0x80000000)) {
                D(printf("%s reseting, anding clockgate\n", __func__);)
                s->r[USBPHY_CTRL] |= 0x40000000;
            }
            break;
    }
    D(printf("%08x\n", s->r[offset >> 4]);)
}


static const MemoryRegionOps imx23_usbphy_ops = {
    .read = imx23_usbphy_read,
    .write = imx23_usbphy_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static int imx23_usbphy_init(SysBusDevice *dev)
{
    imx23_usbphy_state *s = FROM_SYSBUS(imx23_usbphy_state, dev);

    memory_region_init_io(&s->iomem, &imx23_usbphy_ops, s,
            "imx23_usbphy", 0x2000);
    sysbus_init_mmio(dev, &s->iomem);

    s->r[USBPHY_PWD] = 0x00860607;
    s->r[USBPHY_PWD] = 0x00860607;
    s->r[USBPHY_CTRL] = 0xc0000000;
    return 0;
}


static void imx23_usbphy_class_init(ObjectClass *klass, void *data)
{
    SysBusDeviceClass *sdc = SYS_BUS_DEVICE_CLASS(klass);

    sdc->init = imx23_usbphy_init;
}

static TypeInfo usbphy_info = {
    .name          = "imx23_usbphy",
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(imx23_usbphy_state),
    .class_init    = imx23_usbphy_class_init,
};

static void imx23_usbphy_register(void)
{
    type_register_static(&usbphy_info);
}

type_init(imx23_usbphy_register)

