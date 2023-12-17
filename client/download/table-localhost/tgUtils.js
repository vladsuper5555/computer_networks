HTTP/1.1 200 OK
Host: localhost:8081
Date: Thu, 14 Dec 2023 21:18:08 GMT
Connection: close
Content-Type: application/javascript
Content-Length: 5660

function addTargetIdsToTabs(targets, tabs) {
    // WARNING: will also return tabs with target id not found (if any), for now at least
    const tabsWithTargetIds = [];
    for (const tab of tabs) {
        const foundTarget = targets.find((target) => {
            return target.tabId === tab.id;
        });
        if (foundTarget) {
            const temp = { ...tab, targetId: foundTarget.id };
            tabsWithTargetIds.push(temp);
            continue;
        }
        tabsWithTargetIds.push(tab);
    }
    return tabsWithTargetIds;
}

async function whoAmI() {
    //const testResult = await testAskQueryAll();
    const recipient = {
        '@type': 'Application',
        'name': null,
        'id': { browserId: null, targetId: null }, // targetId might be used in the future for amazon query tabgram
    };
    const result = await tabgram.ask({
        payload: {
            domain: { name: 'tabgram' }, // maybe domain: "BrowserExtension", w/o {name:""} ??
            api: 'interstitial',
            method: 'whoami',
            params: null,
        },
        recipient,
    });
    return result;
}

//todo: this call is sending from tgApp two requests to the extension, this justifies a native cmd in the extension and making it one call
async function getTabsWithTargetIds(recipientBrowserId) {


    const recipient = {
        '@type': 'Application',
        'name': 'BrowserExtension',
        'id': { browserId: recipientBrowserId, targetId: 'BrowserExtension' },
    };


    const tabsResult = await tabgram.ask({
        payload: {
            domain: { name: 'BrowserExtension' },
            api: 'tabs',
            method: 'query',
            params: {"test":"12345"},
        },
        recipient,
    });


    const targetsResult = await tabgram.ask({
        payload: {
            domain: { name: 'BrowserExtension' },
            api: 'debugger',
            method: 'getTargets',
            params: null,
        },
        recipient,
    });


    //if (tabsResult && targetsResult) {
    if (tabsResult && targetsResult && (!tabsResult.jsonPayload.hasOwnProperty("error"))){ //todo: test for error would have to test both and test error for true
        tabs = addTargetIdsToTabs(targetsResult.jsonPayload.result, tabsResult.jsonPayload.result);
        if (!Array.isArray(tabs) || tabs.length < 1) {
            return null;
        }
        return tabs;
    } else {
        return {error:true}
    }
}

async function getTabTargetId(tab, recipientBrowserId) {
    const recipient = {
        '@type': 'Application',
        'name': 'BrowserExtension',
        'id': { browserId: recipientBrowserId, targetId: 'BrowserExtension' },
    };
    const targetsResult = await tabgram.ask({
        payload: {
            domain: { name: 'BrowserExtension' },
            api: 'debugger',
            method: 'getTargets',
            params: null,
        },
        recipient,
    });
    const foundTarget = targetsResult.jsonPayload.result.find((target) => {
        return target.tabId === tab.id;
    });
    return foundTarget.id;
}

async function addTargetIdToTab(tab, recipientBrowserId) {
    const tabTargetId = await getTabTargetId(tab, recipientBrowserId);
    if (tabTargetId) {
        const temp = { ...tab, targetId: tabTargetId };
        return temp;
    }
    return tab;
}

// function to generate new subscription ids
function guidGenerator() {
    var S4 = function () {
        return (((1 + Math.random()) * 0x10000) | 0).toString(16).substring(1);
    };
    return S4();
}

// WARNING: INTERNAL helper function, only to be used by tgSociety/tgGateway/etc, not by tabgrams
async function runQueryScript(querySpecs, recipient) {
    let script;
    switch (querySpecs?.method?.toLowerCase()) {
        case 'queryselectorall':
            //script = `(function t(){let elems = Array.from(document.${querySpecs?.method}('${querySpecs?.selector}')); let items = elems.map(x => x.${querySpecs?.element}); return items }())`;
            script = `(function t(){let elems = Array.from(document.${querySpecs?.method}('${querySpecs?.selector}')); let items = []; if (elems[0] != null)  {items = elems.map(x => x.${querySpecs?.element});}; return items }())`;
            break;
        case 'queryselector':
            //script = `(function t(){let elems = Array.of(document.${querySpecs?.method}('${querySpecs?.selector}')); let items = elems.map(x => x.${querySpecs?.element}); return items }())`;
            script = `(function t(){let elems = Array.of(document.${querySpecs?.method}('${querySpecs?.selector}')); let items = []; if (elems[0] != null)  {items = elems.map(x => x.${querySpecs?.element});}; return items }())`;
            break;
    }

    try {
        const askResult = await tabgram.ask(
            // This below is THE important step - tgSociety is transforming jsonpayload without the asking tabgram having to care about
            {
                payload: {
                    domain: { name: 'BrowsersController' },
                    api: 'browser',
                    method: 'evaluateInTab',
                    params: script,
                },
                recipient: recipient,
            },
        );
        let returnValue = { targetId: recipient.id.targetId, title: querySpecs.title, ...askResult.jsonPayload };
        //return { targetId: recipient.id.targetId, title: querySpecs.title, ...askResult.jsonPayload };
        return returnValue;
    } catch (e) {
        console.log('ERRRRRRRRRRRRRRRRRRRRRRRRRRRRR', e);
    }
}

const timeoutPromise = (prom, time) => Promise.race([prom, new Promise((_r, rej) => setTimeout(rej, time))]);
