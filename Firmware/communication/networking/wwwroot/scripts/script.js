class Router {
  constructor(routes, el) {
    this.routes = routes;
    this.el = el;
    window.onhashchange = this.hashChanged.bind(this);
    this.hashChanged();
  }

  async hashChanged(ev) {
    if (window.location.hash.length > 0) {
      const pageName = window.location.hash.substr(1);
      this.show(pageName);
    } else if (this.routes['#default']) {
      this.show('#default');
    }
  }

  async show(pageName) {
    const page = this.routes[pageName];
    await page.load();
    this.el.innerHTML = '';
    page.show(this.el);
  }
}

class Page {
  constructor(url) {
    this.url = 'pages/' + url;
  }

  load() {
    return fetch(this.url)
        .then(res => res.text())
        .then(res => this.html = res);
  }  

  show(el) {
    el.innerHTML = this.html;
  }
}
class Layout {
  constructor(...pages) {
    this.pages = pages;
  }

  load() {
    return Promise.all(this.pages.map(page => page.load()));
  }

  show(el) {
    for (let page of this.pages) {
      const div = document.createElement('div');
      page.show(div);
      el.appendChild(div);
    }
  }
}

const r = new Router(
  {
    '#default': new Page('index.html'),
    scope: new Layout(new Page('scope.html')),
    xy: new Layout(new Page('xy.html')),
    api: new Layout(new Page('api.html')),
    calibration: new Layout(new Page('calibration.html')),
    manual: new Layout(new Page('manual.html')),
    help: new Layout(new Page('help.html')),
  },
  document.querySelector('main')
);



//////////// ODrive API Connector

class ODrive {
  constructor(url = '/api') {
    this.url = url;
  }

  
  async connect() {
    this.schema = await (await fetch(this.url)).json();
  }

  async setParam(param, value) {
    // The future...
    //return (await fetch(this.url + '/api/' + param, {method: 'POST', body: value})).text()
  }

  async getParam(...params) {
    return this.scopeParam(0, 1, ...params)[0];
  }

  async scopeParam(interval, count, ...params) {
    // The future...
    //return fetch(this.url + '/api/scope' + param, {method: 'POST', body: params})

    // The now...   browser does timekeeping...  badly...
    var all_results = [];
    for (var i=0; i< count; i++) {

      var requests = params.map(
        param => fetch(this.url + '/' + param).then(x => x.text())
        );

      requests.push(timeout(interval));

      var results = await Promise.all(requests);

      results.pop();
      all_results.push(results);
    }
    return all_results;
  }
}

const o = new ODrive();





///////////// UTILS

function timeout(s) {
    return new Promise(resolve => setTimeout(resolve, s*1000));
}
