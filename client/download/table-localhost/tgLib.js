HTTP/1.1 200 OK
Host: localhost:8081
Date: Thu, 14 Dec 2023 21:18:08 GMT
Connection: close
Content-Type: application/javascript
Content-Length: 30595

//tgLib.js as class with a direct instanciation because I could not make it work as a module

//still missing reply action: means currently no tabgram can be receiving an ask and send a reply

//var fetchTgApiUrl = 'https://tabgram.localhost/test12345/POSTGoogleChrome/puppeteer/master/docs/api.md'

var logReceivedMessages2Debug = true;

var helloMessageFormalistic = true;

const isDebugModeOnTgLib = false;

let subscribers = [] // this are the subscribers that we use for tabgram.onInput
let lastSentMessage; // same reason as above
const onInput = (callback) => {
    subscribers.push(callback);
}

const output = (message) => {
    lastSentMessage = message;
    window.top.postMessage(message, '*')
}
const ask = (askAction) => {
    if (askAction.domain.name == 'tabgramPipe' && askAction.api == 'orchestratorAsk') 
    {
        if (askAction.method == 'changeWindowLocationHref' && askAction.params.url != undefined)
        {
            // send the ask to the orchestrator
            window.top.postMessage(askAction, '*');
        }
    }
}
var tabgramPipe = {
    onInput: onInput,
    output: output,
    ask: ask,    
};

window.addEventListener('message', (e) => {
    if (window.location.href.indexOf('tgContainer') != -1) //  we need to know if this is a container or not in order to do this part or not
        return; 
    if (e.source != window.top)
    {
        e.stopImmediatePropagation()
        tabgramPipe.output('kill');
    }
    else if (e.data == 'resendLastMessage')
    {
        if (lastSentMessage != undefined && lastSentMessage != null)
            tabgramPipe.output(lastSentMessage);
    }
    else
        for (let subscriber of subscribers) 
            subscriber(e.data);
})

class Tabgram extends EventTarget {
    //fetchTgApiUrl = 'https://tabgram.localhost/test12345/POSTGoogleChrome/puppeteer/master/docs/api.md';
    fetchTgApiUrl = 'https://' + window.location.hostname + '/tgapi';
    activeSubscriptions = {};
    informActionWrapper = {
        '@context': {
            '@vocab': 'http://schema.org',
            'jsonPayload': { '@id': 'http://tabgram.com/vocab/json/browserextension/event/instance', '@type': '@json' },
        },
        '@type': 'InformAction',
        'about': {},
        'agent': {
            '@type': 'Application',
            'name': 'BrowserExtension', // ??\
            'id': { targetId: 'BrowserExtension' }, // Potential Error : Why defaulted to browser extension?
        },
        'recipient': {
            id: {},
        },
        'jsonPayload': {},
        'id': {},
    };
    pendingReplies = [];
    identity = {
        browserId: null,
        targetId: null,
    };
    app = null;

    earlyReplies = new Map();
    receivedMessageLog = [];

    async registerApp(appConfig) {
        if (isDebugModeOnTgLib == true)
            console.log('###DEBUG(tgLib.js): register triggered');
        // const appAddress = await this.whoAmI();
        const registerResult = await tabgram.ask({
            payload: {
                domain: { name: 'tabgram' },
                api: 'tabgram',
                method: 'registerApplication',
                params: appConfig,
            },
            recipient: {
                '@type': 'Application',
                'name': 'BrowserExtension',
                'id': { browserId: 0, targetId: 'BrowserExtension' },
            },
        });
        if (isDebugModeOnTgLib == true)
            console.log('###DEBUG(tgLib.js): register result', registerResult);
        this.identity = registerResult.jsonPayload.address;
        return registerResult.jsonPayload.address;
    }

    async whoAmI() {
        if (isDebugModeOnTgLib == true)
            console.log('###DEBUG(tgUtils.js): get self address');
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
        if (isDebugModeOnTgLib == true)
            console.log('###DEBUG(tgUtils.js): get self address result', result);
        this.identity = result.jsonPayload.address; //new
        return result.jsonPayload.address;
    }

    async informAction(recipient, action, agent) {
        return this.genericAction('InformAction', recipient, action, null, agent);
    }

    // TODO: refactor this to make it similar to ask
    async inform(informData) {
        // ?? what URL to use for fetches?
        //return await fetch('https://tabgram.localhost/test12345/POSTGoogleChrome/puppeteer/master/docs/api.md', {
        return await fetch(this.fetchTgApiUrl, {
            method: 'POST',
            headers: {
                'Accept': 'application/json, application/xml, text/plain, text/html, *.*',
                'Content-Type': 'application/x-www-form-urlencoded; charset=utf-8',
            },
            body: JSON.stringify(informData),
        });
    }

    //warning: adding to the mess of calls : tglib needs to be cleaned up
    //new because in tgLib for type3 we need to be able to finetune message (incl about propery)
    //if successful then to be reused in other calls in here
    /*
        return new Promise(async (resolve, reject) => {
            let responseString = await this.askAction(recipient, payload);
            let message = JSON.parse(responseString);

            if (message['@type'] == 'ConfirmAction') {
                console.log('getReply: ConfirmAction, id of original Ask is: ', message.about.id);
                this.pendingReplies.push({ id: message.about.id, resolve: resolve });
            } else {
                console.log('getReply: Immediate reply received ');
                resolve(message);
            }
        });

*/

    async fetchPostAction(action) {
        return new Promise(async (resolve, reject) => {
            let responseString = await fetch(
                //'https://tabgram.localhost/test12345/POSTGoogleChrome/puppeteer/master/docs/api.md',
                this.fetchTgApiUrl,
                {
                    method: 'POST',
                    headers: {
                        'Accept': 'application/json, application/xml, text/plain, text/html, *.*',
                        'Content-Type': 'application/x-www-form-urlencoded; charset=utf-8',
                    },
                    body: JSON.stringify(action),
                },
            )
                .then(async function (response) {
                    let x = response;
                    let y = await response.text();
                    return y;
                })
                .catch((error) => {
                    if (isDebugModeOnTgLib == true)
                        console.log('Error:', error);
                    return { error: true };
                });

            let message = JSON.parse(responseString);

            if (message['@type'] == 'ConfirmAction') {
                if (isDebugModeOnTgLib == true)
                    console.log('getReply: ConfirmAction, id of original Ask is: ', message.about.id);
                this.pendingReplies.push({ id: message.about.id, resolve: resolve });
            } else {
                if (isDebugModeOnTgLib == true)
                    console.log('getReply: Immediate reply received ');
                resolve(message);
            }
        });
    }

    //special case: ReplyAction requires additional param about (the original ask)
    async genericAction(actionType, recipient, payload, about = null, agent = null) {
        let i = structuredClone(this.informActionWrapper);

        i['@type'] = actionType;
        i['recipient'] = recipient;
        i.jsonPayload = payload;

        //new
        if (about != null) {
            i.about = about;
        }
        if (agent != null) {
            i.agent = agent;
        }

        //return fetch('https://localhost:3000/test12345/POSTGoogleChrome/puppeteer/master/docs/api.md', {
        //return fetch('https://tabgram.localhost/test12345/POSTGoogleChrome/puppeteer/master/docs/api.md', {
        return fetch(this.fetchTgApiUrl, {
            method: 'POST',
            headers: {
                'Accept': 'application/json, application/xml, text/plain, text/html, *.*',
                'Content-Type': 'application/x-www-form-urlencoded; charset=utf-8',
            },
            body: JSON.stringify(i),
        })
            .then(async function (response) {
                let x = response;
                let y = await response.text();
                return y;
            })
            .catch((error) => {
                if (isDebugModeOnTgLib == true)
                    console.log('Error:', error);
                return { error: true };
            });
    }

    async askAction(recipient, askPayload) {
        return this.genericAction('AskAction', recipient, askPayload);
    }

    // lets say bellow are / should be the public / user-ending methods of tabgram lib (and private above)

    // ?? SHOULD THIS BE HERE in the consumer lib or move it on an internal lib since it is used only by tgSociety?

    //warning: authorizeaction has the property "object" to be used for the thing that is to be authorized
    //because we use the generic the object ie action that is authorized will be wrapped not in an "object" property but in jsonPayload!
    async authorizeAction(recipient, action) {
        return this.genericAction('AuthorizeAction', recipient, action);
    }

    async getPotentialActions() {
        const ask = {
            domain: { name: 'tabgram' },
            api: 'interstitial',
            method: 'queryPotentialActions',
            data: null,
        };
        const recipient = {
            '@type': 'Application',
            'name': 'Tabgram',
            'id': {},
        };
        let potentialActionsReplyString = await this.askAction(recipient, ask); //dummy event needs to be adjusted to express ask!
        let potentialActionsReply = JSON.parse(potentialActionsReplyString);
        let potentialAction = potentialActionsReply.jsonPayload;

        //for later potential optimisation: put the code below into a function that is called from brwoserControl directly with the potentialactions as parameters
        //this will save that first from browsercontrol event for triggering getPotentialActions triggered and then potentialAction is fetched (directly in the lines above thats what is happening)
        if (isDebugModeOnTgLib == true)
            console.log('PotentialActions retrieved:');
        if (typeof potentialAction != 'undefined') {
            if (isDebugModeOnTgLib == true)
                console.log(potentialAction);
            if (potentialAction['@type'] == 'ReplyAction') {
                let pendingReply = this.pendingReplies.find((x) => potentialAction.about.id == x.id);
                if (typeof pendingReply != 'undefined') {
                    if (isDebugModeOnTgLib == true)
                        console.log('pending reply about to be resolved');
                    pendingReply.resolve(potentialAction);
                }
            }
            if (potentialAction['@type'] == 'AllocateAction') {
                if (isDebugModeOnTgLib == true)
                    console.log('About to issue event -newAllocateAction-');
                //tabgram.dispatchEvent(new Event('newAllocateAction'));
                tabgram.dispatchEvent(new CustomEvent('newAllocateAction', { detail: potentialAction }));
            }
            if (potentialAction['@type'] == 'AskAction') {
                if (isDebugModeOnTgLib == true)
                    console.log('About to issue event -newAskAction-');
                //tabgram.dispatchEvent(new Event('newAllocateAction'));
                tabgram.dispatchEvent(new CustomEvent('newAskAction', { detail: potentialAction }));
            }
            if (potentialAction['@type'] == 'InformAction') {
                if (isDebugModeOnTgLib == true)
                    console.log('About to issue event -newInformAction-');
                tabgram.dispatchEvent(new CustomEvent('newInformAction', { detail: potentialAction }));
            }
            if (potentialAction['@type'] == 'CommunicateAction') {
                if (isDebugModeOnTgLib == true)
                    console.log('About to issue event -newCommunicateAction-');
                tabgram.dispatchEvent(new CustomEvent('newCommunicateAction', { detail: potentialAction }));
            }
        } else {
            if (isDebugModeOnTgLib == true)
                console.log('potential error: retrieved potentialAction is undefined');
        }
    }

    async getPotentialActionNew(potentialAction) {

        /*
        //enabled to understand during debugging why there is a difference
        const ask = {
            domain: { name: 'tabgram' },
            api: 'interstitial',
            method: 'queryPotentialActions',
            data: null,
        };
        const recipient = {
            '@type': 'Application',
            'name': 'Tabgram',
            'id': {},
        };
        let potentialActionsReplyString = await this.askAction(recipient, ask); //dummy event needs to be adjusted to express ask!
        let potentialActionsReply = JSON.parse(potentialActionsReplyString);
        let potentialActionOld = potentialActionsReply.jsonPayload;
        console.log("originalFetched potentialAction:", potentialActionOld);
        */

        //for later potential optimisation: put the code below into a function that is called from brwoserControl directly with the potentialactions as parameters
        //this will save that first from browsercontrol event for triggering getPotentialActions triggered and then potentialAction is fetched (directly in the lines above thats what is happening)
        if (isDebugModeOnTgLib == true)
            console.log('getPotentialActionNew retrieved :-) as param:', potentialAction);
        if (typeof potentialAction != 'undefined') {
            if (isDebugModeOnTgLib == true)
                console.log(potentialAction);
            if (potentialAction['@type'] == 'ReplyAction') {
                let pendingReplyIndex = this.pendingReplies.findIndex((x) => potentialAction.about.id == x.id);
                if (pendingReplyIndex !== -1) {
                    if (isDebugModeOnTgLib == true)
                        console.log('pending reply about to be resolved');
                    this.pendingReplies[pendingReplyIndex].resolve(potentialAction);
                    this.pendingReplies.splice(pendingReplyIndex, 1);
                } else {
                    //new for changed flow of getPotentialActions : it can happen that a reply is faster here then registartion of the the id through confirmaction
                    this.earlyReplies.set(potentialAction.about.id, potentialAction);
                }
            }
            if (potentialAction['@type'] == 'AllocateAction') {
                if (isDebugModeOnTgLib == true)
                    console.log('About to issue event -newAllocateAction-');
                //tabgram.dispatchEvent(new Event('newAllocateAction'));
                tabgram.dispatchEvent(new CustomEvent('newAllocateAction', { detail: potentialAction }));
            }
            if (potentialAction['@type'] == 'AskAction') {
                if (isDebugModeOnTgLib == true)
                    console.log('About to issue event -newAskAction-');
                //tabgram.dispatchEvent(new Event('newAllocateAction'));
                tabgram.dispatchEvent(new CustomEvent('newAskAction', { detail: potentialAction }));
            }
            if (potentialAction['@type'] == 'InformAction') {
                if (isDebugModeOnTgLib == true)
                    console.log('About to issue event -newInformAction-');
                tabgram.dispatchEvent(new CustomEvent('newInformAction', { detail: potentialAction }));
            }
            if (potentialAction['@type'] == 'CommunicateAction') {
                if (isDebugModeOnTgLib == true)
                    console.log('About to issue event -newCommunicateAction-');
                tabgram.dispatchEvent(new CustomEvent('newCommunicateAction', { detail: potentialAction }));
            }
            if (logReceivedMessages2Debug){
                this.receivedMessageLog.push(potentialAction)
            }

        } else {
            if (isDebugModeOnTgLib == true)
                console.log('potential error: retrieved potentialAction is undefined');
        }
    }

    //Important: currently waiting forever to get reply; better to offer third parameter timeout
    //especially important because tgSociety calling through this tgInteractions and if tgInteractions would not answer this will lead to a stalled action with no trace
    async askRecipient(recipient, payload) {
        //missing: the obvios check of the payload json structure

        if (recipient != null) {
            return this.ask({ payload, recipient });
        } else {
            return { error: true };
        }
    }

    async replyRecipient(recipient, reply, ask) {
        return this.genericAction('ReplyAction', recipient, reply, ask);
    }

    //parameter handling needs cleanup, not requred here ?
    //Important: currently this is THE ask routine to be called because only here we push to pendingReplies!
    async ask(askData) {
        // TODO: add validation for the structure of all 3 supported payload types
        let recipient, payload;
        // FIXME: this is limiting the true capabilities of the ask actions to only 1:1 and [1:1]
        // we also support 1:M down the line (tgSociety) but this is not yet implemented here
        if (Array.isArray(askData)) {
            recipient = askData.map((el) => el.recipient);
            payload = askData.map((el) => el.payload);
        } else {
            recipient = askData.recipient;
            payload = askData.payload;
        }

        return new Promise(async (resolve, reject) => {
            let responseString = await this.askAction(recipient, payload);
            let message = JSON.parse(responseString);

            if (message['@type'] == 'ConfirmAction') {
                if (isDebugModeOnTgLib == true)
                    console.log('getReply: ConfirmAction, id of original Ask is: ', message.about.id);
                //new: because of changed flow for getPotentialActions a reply could be already available in earlyReplies although we just receive here tthe id of the to be expected reply !
                if (this.earlyReplies.has(message.about.id)) {
                    resolve(this.earlyReplies.get(message.about.id))
                } else {
                    this.pendingReplies.push({ id: message.about.id, resolve: resolve });
                }

            } else {
                if (isDebugModeOnTgLib == true)
                    console.log('getReply: Immediate reply received ');
                resolve(message);
            }
        });

        //return message
    }

    async subscribe(...args) {
        // TODO: the obvious check of arguments structure and QC like: cb a fct?
        const callback = args[args.length - 1];
        const subscribePayloads = args.slice(0, args.length - 1);
        // mainly for communicate subscribe
        // maybe split this into different functions for subscribe and communicate subscribe
        const subscribeCriteria = subscribePayloads[0];
        if (!subscribeCriteria.hasOwnProperty('recipient')) {
            //warning: it would be better to check if defaulting is making sense if recipient is missing
            //example: on a fetch .subscribe defaulting here recipient is NOT making sense and an error shhould be provided
            //example: on commuicate subscribe is making sense as recipient has no meaning for it
            subscribeCriteria.recipient = {
                //could this here be defaulted?
                '@type': 'Application',
                'name': 'tgSociety',
                'id': { browserId: this.identity.browserId, targetId: 'tgSociety' },
            };
            subscribePayloads[0] = subscribeCriteria;
        }
        // CHECK THIS PAYLOAD STRUCTURE
        const reply = await this.ask(subscribePayloads);
        if (reply?.jsonPayload.hasOwnProperty('subscriptionId')) {
            const subId = reply.jsonPayload.subscriptionId; //warning: maybe subscriptions could also fail then here we need to perpare for unhappy path
            this.activeSubscriptions[subId] = callback;
        } else {
            if (isDebugModeOnTgLib == true)
                console.error(
                    'Fatal error: Subscribe Ask received reply but subscriptionId not part of it!',
                    subscribePayloads,
                    reply,
                );
        }
        return reply;
    }

    async unsubscribe(subscriptionId) {
        if (isDebugModeOnTgLib == true)
            console.log('###DEBUG: spec sent for unsubscribe reply');
        const unsubAskPayload = {
            payload: {
                domain: { name: 'tabgram' },
                api: 'tgSociety',
                method: 'unsubscribe',
                params: { subscriptionId },
            },
            recipient: {
                '@type': 'Application',
                'name': 'Tabgram',
                'id': { browserId: null, targetId: 'Tabgram' },
            },
        };
        const reply = await this.ask(unsubAskPayload);
        if (reply.jsonPayload.successful === true) {
            delete this.activeSubscriptions[subscriptionId];
        }
        return reply;
    }

    //maybe to be removed
    async communicateAction(recipient, payload) {
        if (payload.method === 'informAction') {
            payload.method = 'onInformAction'; //this feels not the best place but is required so that the current implementation of the subscription mechanism in tgSociety can be leveraged
        }
        return this.genericAction('CommunicateAction', recipient, payload);
    }

    //fyi: willingness to listen to this type of .communication requires inside a tabgram just the right .subscribe to the api communication
    async communicate(
        payload,
        callback = null,
        about = null,
        recipient = { '@type': 'Application', 'name': 'tabgram', 'id': { browserId: 1, targetId: null } },
    ) {
        /*
        const recipient = {
            '@type': 'Application',
            'name': 'tabgram',
            'id': { browserId: 1, targetId: null }, // targetId might be used in the future for amazon query tabgram
        };
        */
        payload.domain = { name: 'tabgram' }; // maybe domain: "BrowserExtension", w/o {name:""} ??
        payload.api = 'communication';

        if (payload.method === 'informAction') {
            payload.method = 'onInformAction'; //this feels not the best place but is required so that the current implementation of the subscription mechanism in tgSociety can be leveraged

            //experiment: see the impact of making hello messages fit to the structure of the other type3 messages
            if (helloMessageFormalistic) {
                payload.params = {
                    specifications: [
                        {
                            input: payload.params,
                        },
                    ],
                };
            }

            return this.genericAction('CommunicateAction', recipient, payload);
        }
        if (payload.method === 'askAction') {
            //payload.method =  "onAskAction" //this feels not the best place but is required so that the current implementation of the subscription mechanism in tgSociety can be leveraged
            //next steps here :-)
            payload.events = ['onAskAction']; //this means that in tgSociety this will land as a "true" .subscribe and that means in tgSociety we have to specifically identify this message because it is not just another subscription!
            //it is just using the subscription mechanism to be able receive from potentially multiple tabgrams subscribed to this askAction replies through the subscription mechanism.
            //other than that in tgSociety it will not enter into the subscriber list, instead there will be a specif routine:
            //1) searching for subscriber to this commuication .ask
            //2) send them ... tbd
            //important: listeners to asks in Communication are to be setup through the existing .subscribe !
            payload.method = 'subscribe'; //either this or remove property so that tgSociety routing works
            return this.subscribe({ payload, recipient }, callback);
        }

        // "replyAction" required too!
        if (payload.method === 'replyAction') {
            return this.genericAction('CommunicateAction', recipient, payload, about);
        }

        return { error: true };
    }
}
const tabgram = new Tabgram();


// ( () => {
//     tabgram.identity = tabgram.whoAmI(); //todo: Interesting :-) why do we have to call tabgram.whoamI at all ?? if its done during initialisation of the library
// })();


//tabgram.dispatchEvent(new Event('newAction')); // script to be evaluated by puppeteer on page

tabgram.addEventListener('newPotentialAction', () => {
    //console.log("newpotentialAction event triggered with parameter m:", m);
    if (isDebugModeOnTgLib == true)
        console.log('I could fetch now new actions from localhost/tabgram');
    tabgram.getPotentialActions();
});

tabgram.addEventListener('newInformAction', async (e) => {
    if (isDebugModeOnTgLib == true)
        console.log('###DEBUG: e:', e);

    if (e.detail.about.subscriptionId && !!tabgram.activeSubscriptions[e.detail.about.subscriptionId]) {
        if (isDebugModeOnTgLib == true)
            console.log('###DEBUG: executing callback for informAction subscription', e.detail.about.subscriptionId);
        // IDEA to have something here to also store the messages to be able to be retrieved later on or in case a callback is not provided for a subscriptionS
        return await tabgram.activeSubscriptions[e.detail.about.subscriptionId](e.detail.jsonPayload);
    } else {
        if (isDebugModeOnTgLib == true)
            console.error("No activeSubscription found for received informAction:", e.detail)
    }
    // why this because is not working in the means of undefined value
    if (tabgram.activeSubscriptions[e.detail.about.subscriptionId] != undefined)
        if (!!!tabgram.activeSubscriptions[e.detail.about.subscriptionId]) {
            await tabgram.unsubscribe(e.detail.about.subscriptionId);
        }

    // use the list of subs. above to find the callback based on the subscription id
    // e should have the subscription Id
    // once the desired tuple of {subs, cb} is found, run cb(e.detail.jsonPayload)
});

//this is for now identical to newInformAction BUT it will be likely different when onAsk or onRequest type3 message needs to be handled here corectly
tabgram.addEventListener('newCommunicateAction', async (e) => {
    if (isDebugModeOnTgLib == true)
        console.log('###DEBUG: e:', e);

    if (e.detail.about.subscriptionId && !!tabgram.activeSubscriptions[e.detail.about.subscriptionId]) {
        if (isDebugModeOnTgLib == true)
            console.log('###DEBUG: executing callback for communicateAction subscription', e.detail.about.subscriptionId);
        return await tabgram.activeSubscriptions[e.detail.about.subscriptionId](e.detail.jsonPayload?.params); //to what and why do we return here anything? and why awaiting?
    }

    // use the list of subs. above to find the callback based on the subscription id
    // e should have the subscription Id
    // once the desired tuple of {subs, cb} is found, run cb(e.detail.jsonPayload)
});

tabgram.addEventListener('newAskAction', async (e) => {
    if (isDebugModeOnTgLib == true)
        console.log('###DEBUG: e:', e);

    if (e.detail.about.subscriptionId && !!tabgram.activeSubscriptions[e.detail.about.subscriptionId]) {
        if (isDebugModeOnTgLib == true)
            console.log('###DEBUG: executing callback for askAction subscription', e.detail.about.subscriptionId);
        let payload = await tabgram.activeSubscriptions[e.detail.about.subscriptionId](e.detail.jsonPayload);
        /*
        const res = await tabgram.communicate(  // alternatively this could be a .communicateAction with the method subscribeAction ; would be in harmony with the inform. Could be internally already in tgLIb translated to the current .subscribe
            {
                //domain: { name: 'tabgram' },
                //api: 'communication',
                method: 'replyAction',
                params: {
                    "about": e.detail, // through this in tgSociety identify where it should go.... by what field in askAction? ; in principal this could be used to send not only a one time reply but instead a stream of replies ; there are scenarios where this could be helpfull ("still alive"), ideally this could be expressed once/stream in the input schema
                    "jsonPayload" : payload
                }
            }
        );
        */

        // trying to be even more simple - just send a reply
        // maybe could use: async genericAction(actionType, recipient, payload, about = null) {

        const recipient = {
            '@type': 'Application',
            'name': 'Tabgram',
            'id': {},
        };

        let i = structuredClone(tabgram.informActionWrapper);

        i['@type'] = 'ReplyAction';
        i.about = e.detail;
        i.recipient = recipient;
        i.jsonPayload = payload;

        let x = await tabgram.fetchPostAction(i);
        if (isDebugModeOnTgLib == true)
            console.log('reply to ask sent, response to sending it:', x);

        //return res //nonsense right now if we get here we need to send
    }

    // use the list of subs. above to find the callback based on the subscription id
    // e should have the subscription Id
    // once the desired tuple of {subs, cb} is found, run cb(e.detail.jsonPayload)
});
