import sys, os, struct;

sep = struct.pack("BBBB", 0, 0, 0, 1)

def countNalus(inFileName, type=20):
    stream = []
    cnt = 0
    with open(inFileName, 'rb') as fpIn:
        stream = fpIn.read().split(sep)[1:]
    for i in range(len(stream)):
        n = stream[i]
        hdr = struct.unpack_from("BBBB", n)
        naluType = hdr[0] & 0x1f
        if naluType == type:
            cnt = cnt + 1
    return cnt
    

def mux(fpOut, layerList, sepNaluType):
    naluStreams = {}
    positions = {}
    nLayers = len(layerList)
    for i in range(nLayers):
        with open(layerList[i]['Filename'], 'rb') as fpIn:
            naluStreams[i] = fpIn.read().split(sep)[1:]
            positions[i] = 0
    
    active = True
    baseLayerAUCount = layerList[0]['naluCount']
    frm = 0
    nal_ref_idc = 0
    while active:
        active = False
        for i in range(nLayers):
            naluPerAU = layerList[i]['naluCount'] / baseLayerAUCount
            eos, found = False, False
            first = True
            if i == 0:
                cnt = 0
                while (not eos) and (not found):
                    pos = positions[i]
                    if pos >= len(naluStreams[i]):
                        eos = True
                    else:
                        n = naluStreams[i][pos]
                        if (len(n) > 0):
                            nal_ref_idc = struct.unpack_from("B", n)[0] >> 5
                            naluType = struct.unpack_from("B", n)[0] & 0x1f
                            if naluType == sepNaluType:
                                if not first and nLayers > 1:
                                    found = True
                                    active = True
                            first = False
                        if naluType != 14:
                            cnt += 1
                        if not found:
                            fpOut.write(sep+n)
                            positions[i] +=  1
                            if nLayers == 1 and naluType == 1 and (nal_ref_idc == 2 or (nal_ref_idc == 0 and cnt >= naluPerAU)):
                                active = True
                                found = True
                frm += 1
            else:
                cnt = 0
                while (not eos) and cnt < naluPerAU:
                    pos = positions[i]
                    if pos >= len(naluStreams[i]):
                        eos = True
                        active = False
                    else:
                        n = naluStreams[i][pos]
                        cnt += 1
                        nal_ref_idc = struct.unpack_from("B", n)[0] >> 5
                        naluType = struct.unpack_from("B", n)[0] & 0x1f
                        fpOut.write(sep+n)
                        positions[i]+= 1
                        active = True


def merge(layerGroup, outFile, initLayer):
    numLayers = len(layerGroup)
    fpInit = open(initLayer, 'rb')
    initLayerContent = fpInit.read()
    fpInit.close()
    files = {}
    separatorNaluType = 6
    for i in range(numLayers):
        curFile = layerGroup[i]
        naluCount = 0
        if i == 0:
            naluCount = countNalus(curFile, 6)
            if naluCount == 0:
                naluCount = countNalus(curFile, 14)
                separatorNaluType = 14
        else:
            naluCount = countNalus(curFile, 20)
        files[i] = {'Filename': curFile, 'naluCount': naluCount}
    fpOut = open(outFile, 'wb')
    fpOut.write(initLayerContent)
    mux(fpOut, files, separatorNaluType)
    fpOut.close()
